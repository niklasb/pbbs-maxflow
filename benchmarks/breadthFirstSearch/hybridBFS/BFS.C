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
//#include "MatVec.h"
using namespace std;

// **************************************************************
//    HYBRID BREADTH FIRST SEARCH
// **************************************************************

// **************************************************************
//    Switches to Dense matrix-vector multiply when frontier size
//    is large enough
// **************************************************************

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

struct getFrontierSize {
  vertex<intT>* G;
  intT* Visited, *NextVisited;
  getFrontierSize(vertex<intT>* Gr, intT* _Visited, intT* _NextVisited) : 
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? 1 : 0);}
};

struct getFrontierDegreeDense {
  vertex<intT>* G;
  intT* Visited, *NextVisited;
  getFrontierDegreeDense(vertex<intT>* Gr, intT* _Visited, intT* _NextVisited) : 
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? G[i].degree : 0);}
};

pair<intT,intT> BFS(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Frontier = newA(intT,numEdges);
  intT* Visited = newA(intT,numVertices);
  intT* NextVisited = newA(intT,numVertices);
  intT* FrontierNext = newA(intT,numEdges);
  intT* Counts = newA(intT,numVertices);
  intT* Parents = newA(intT,numVertices); //parent pointers of BFS tree
  {parallel_for(intT i = 0; i < numVertices; i++) {
      Visited[i] = 0;
      NextVisited[i]=0;
      Counts[i] = 0;
      Parents[i] = -1; //unvisited nodes retain -1 value
    }
  }
  Frontier[0] = start;
  Parents[start] = start; //parent of root is itself
  intT frontierSize = 1;
  Visited[start] = 1;

  intT totalVisited = 0;
  int round = 0;
  int threshold = (numVertices+numEdges)/10;
  cout<<"threshold = "<<threshold<<endl;
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
	  if(NextVisited[i] != Visited[i]) Counts[i] = 1;
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

      cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-sparse method\n";

      // For each vertexB in the frontier try to "hook" unvisited neighbors.
      {parallel_for(intT i = 0; i < frontierSize; i++) {
	  intT v = Frontier[i];
	  intT o = Counts[i];
	  for (intT j=0; j < G[v].degree; j++) {
	    intT ngh = G[v].Neighbors[j];
	    if (Visited[ngh] == 0 && utils::CAS(&Visited[ngh],(intT)0,(intT)1)) 
	      {
		FrontierNext[o+j] = ngh; 
		Parents[ngh] = i;
	      }
	    else FrontierNext[o+j] = -1;
	    //if(Parents[ngh] == -1) Parents[ngh] = i; //conditional "add"
	  }
	}}

      // Filter out the empty slots (marked with -1)
      frontierSize = sequence::filter(FrontierNext,Frontier,frontierEdgeCount,nonNegF());

      parallel_for(intT i=0;i<frontierSize;i++)
	Counts[i]=G[Frontier[i]].degree;
      frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);

    }
    else {
      //spsv
      cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n";

      parallel_for(intT i=0;i<numVertices;i++){
	NextVisited[i] = Visited[i];
	//if(!NextVisited[i]){
	  for(intT j=0;j<G[i].degree;j++){
	    intT ngh = G[i].Neighbors[j];
	    // NextVisited[i] |= Visited[ngh];
	    // if(Parents[i] != -1) Parents[i] = ngh;
	    if(Visited[ngh]) {
	      //if(Parents[i] == -1) //conditional "add"
	      Parents[i] = ngh;
	      NextVisited[i] = 1;
	      break;
	    }
	  }
	  //}
      }

      frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,NextVisited));
      
      frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),
					    getFrontierSize(G,Visited,NextVisited));
      
      if(frontierSize+frontierEdgeCount < threshold) fromSMV=1;
      swap(NextVisited,Visited);
    }
  }
  free(FrontierNext); free(Frontier); free(Counts); free(Visited); 
  free(Parents);
  free(NextVisited);
  return pair<intT,intT>(totalVisited,round);
}


