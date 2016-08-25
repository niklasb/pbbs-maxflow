// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
using namespace std;

// **************************************************************
//    BETWEENNESS CENTRALITY
// **************************************************************

// **************************************************************
//    Switches to Dense matrix-vector multiply when frontier size
//    is large enough
// **************************************************************

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

struct getFrontierSize {
  vertex<intT>* G;
  double* Visited, *NextVisited;
  getFrontierSize(vertex<intT>* Gr, double* _Visited, double* _NextVisited) : 
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? 1 : 0);}
};

struct getFrontierDegreeDense {
  vertex<intT>* G;
  double* Visited, *NextVisited;
  getFrontierDegreeDense(vertex<intT>* Gr, double* _Visited, double* _NextVisited) : 
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? G[i].degree : 0);}
};

double* BC_hybrid(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Frontier = newA(intT,numEdges);
  intT* FrontierNext = newA(intT,numEdges);
  intT* Counts = newA(intT,numVertices);
  double* NumPaths = newA(double, numVertices);
  double* nextNumPaths = newA(double,numVertices);
  {parallel_for(intT i = 0; i < numVertices; i++) {
      Counts[i] = 0;
      NumPaths[i] = nextNumPaths[i] = 0.0;
    }
  }
  Frontier[0] = start;
  NumPaths[start] = nextNumPaths[start] = 1.0; 
  intT frontierSize = 1;

  intT totalVisited = 0;
  int round = 0;
  int threshold = (numVertices+numEdges)/10;
  //cout<<"threshold = "<<threshold<<endl;
  bool fromSMV = 0;
  intT frontierEdgeCount = G[start].degree;
  while (frontierSize > 0) {
    round++;
    if(frontierSize+frontierEdgeCount< threshold){
      //totalVisited += frontierSize;
      if(fromSMV){
	fromSMV = 0;

	parallel_for(intT i=0;i<numVertices;i++) Counts[i] = 0;
	parallel_for(intT i=0;i<numVertices;i++)
	  if(nextNumPaths[i] != NumPaths[i]) Counts[i] = 1;
	frontierSize = sequence::scan(Counts,Counts,numVertices,utils::addF<intT>(),(intT)0);

	parallel_for(intT i=0;i<numVertices-1;i++){
	  if(Counts[i] != Counts[i+1]) Frontier[Counts[i]] = i;
	}
	if(Counts[numVertices-1] == frontierSize-1)
	  Frontier[frontierSize-1] = numVertices-1;

      {parallel_for (intT i=0; i < frontierSize; i++) 
	  Counts[i] = G[Frontier[i]].degree;}
      frontierEdgeCount = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);
      }

      //cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-sparse method\n";

      // For each vertexB in the frontier try to "hook" unvisited neighbors.
      {parallel_for(intT i = 0; i < frontierSize; i++) {
	  intT v = Frontier[i];
	  intT o = Counts[i];
	  nextNumPaths[v] = NumPaths[v];
	  for (intT j=0; j < G[v].degree; j++) {
	    intT ngh = G[v].Neighbors[j];
	    if(NumPaths[ngh] == 0.0){
	      volatile double oldV, newV; 
	      do {
		oldV = nextNumPaths[ngh]; newV = oldV + NumPaths[v];
	      } while(!utils::CAS(&nextNumPaths[ngh],oldV,newV));
	      if(oldV == 0.0) FrontierNext[o+j] = ngh;
	      else FrontierNext[o+j] = -1;
	    } else FrontierNext[o+j] = -1;

	  }
	}}

      // Filter out the empty slots (marked with -1)
      frontierSize = sequence::filter(FrontierNext,Frontier,frontierEdgeCount,nonNegF());

      parallel_for(intT i=0;i<frontierSize;i++)
      	  Counts[i]=G[Frontier[i]].degree;
      frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);

      swap(NumPaths,nextNumPaths);

    }
    else {
      //spsv
      //cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n";

      parallel_for(intT i=0;i<numVertices;i++){
	nextNumPaths[i] = NumPaths[i];
	if(NumPaths[i]==0.0){
	  for(intT j=0;j<G[i].degree;j++){
	    intT ngh = G[i].Neighbors[j];
	    if(NumPaths[ngh]!=0.0) {
	      nextNumPaths[i] += NumPaths[ngh];
	    }
	  }
	}
      }

      frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,NumPaths,nextNumPaths));
      
      frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(), getFrontierSize(G,NumPaths,nextNumPaths));
      
      if(frontierSize+frontierEdgeCount < threshold) fromSMV=1;
      swap(NumPaths,nextNumPaths);

    }
  }
  free(FrontierNext); free(Frontier); free(Counts); 

  //free(NumPaths);
  free(nextNumPaths);

  return NumPaths;
}

double* BC_sparse(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Frontier = newA(intT,numEdges);
  intT* FrontierNext = newA(intT,numEdges);
  intT* Counts = newA(intT,numVertices);
  double* NumPaths = newA(double, numVertices);
  double* nextNumPaths = newA(double,numVertices);
  {parallel_for(intT i = 0; i < numVertices; i++) {
      Counts[i] = 0;
      NumPaths[i] = nextNumPaths[i] = 0.0;
    }
  }
  Frontier[0] = start;
  NumPaths[start] = nextNumPaths[start] = 1.0; 
  intT frontierSize = 1;

  intT totalVisited = 0;
  int round = 0;
  intT frontierEdgeCount = G[start].degree;
  while (frontierSize > 0) {
    round++;

    parallel_for(intT i=0;i<frontierSize;i++)
      Counts[i]=G[Frontier[i]].degree;
    frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);


    // For each vertexB in the frontier try to "hook" unvisited neighbors.
    {parallel_for(intT i = 0; i < frontierSize; i++) {
	intT v = Frontier[i];
	intT o = Counts[i];
	nextNumPaths[v] = NumPaths[v];
	for (intT j=0; j < G[v].degree; j++) {
	  intT ngh = G[v].Neighbors[j];
	  if(NumPaths[ngh] == 0.0){
	    volatile double oldV, newV; 
	    do {
	      oldV = nextNumPaths[ngh]; newV = oldV + NumPaths[v];
	    } while(!utils::CAS(&nextNumPaths[ngh],oldV,newV));
	    if(oldV == 0.0) FrontierNext[o+j] = ngh;
	    else FrontierNext[o+j] = -1;
	  } else FrontierNext[o+j] = -1;
	}
      }}

    // Filter out the empty slots (marked with -1)
    frontierSize = sequence::filter(FrontierNext,Frontier,frontierEdgeCount,nonNegF());

    swap(NumPaths,nextNumPaths);

  }
  
  free(FrontierNext); free(Frontier); free(Counts); 

  free(nextNumPaths);

  return NumPaths;

}

void paths(intT start, double* ActualNumPaths, graph<intT> G){
  intT* Q = newA(intT,G.n);
  intT start_ptr = 0, end_ptr = 0;
  Q[end_ptr++] = start;
  intT* L = newA(intT,G.n);
  parallel_for(intT i=0;i<G.n;i++) L[i] = -1;
  intT level = 0;
  L[start] = level;
  intT next_end_ptr = 1;
  while(1){
    level++;
    for(intT i=start_ptr;i<end_ptr;i++){
      intT v = Q[i];
      for(intT j=0;j<G.V[v].degree;j++){
	intT ngh = G.V[v].Neighbors[j];
	if(L[ngh] == -1){
	  L[ngh] = level;
	  Q[next_end_ptr++] = ngh;
	  ActualNumPaths[ngh] += ActualNumPaths[v];
	} else if(L[ngh] == level){
	  ActualNumPaths[ngh] += ActualNumPaths[v];
	}
      }
    }
    if(end_ptr == next_end_ptr) break;
    start_ptr = end_ptr; 
    end_ptr = next_end_ptr;
  }
  
  free(Q);
  free(L);
}


void BCCheck(intT start, graph<intT> GA, double* NumPaths){
  intT n = GA.n;
  double* ActualNumPaths = newA(double,n);
  parallel_for(intT i=0;i<n;i++) ActualNumPaths[i] = 0.0;
  ActualNumPaths[start] = 1.0;

  paths(start,ActualNumPaths,GA);

  for(intT i=0;i<n;i++){
    if((intT)NumPaths[i] != (intT)ActualNumPaths[i]){
      cout<<"NumPaths["<<i<<"] wrong "<<NumPaths[i]<<" "<<ActualNumPaths[i]<<endl;
      abort();
    }
  }
  
  free(ActualNumPaths);
}


pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  startTime();
  double* NP1 = BC_hybrid(start,GA);
  nextTime("hybrid");
  double* NP2 = BC_sparse(start,GA);
  nextTime("sparse");

  BCCheck(start,GA,NP1); free(NP1);
  BCCheck(start,GA,NP2); free(NP2);

  return make_pair(0,0);
}
