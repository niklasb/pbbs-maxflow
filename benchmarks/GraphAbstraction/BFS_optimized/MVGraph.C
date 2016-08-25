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


intT* BFS(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Frontier = newA(intT,numEdges);
  // intT* Visited = newA(intT,numVertices);
  // intT* NextVisited = newA(intT,numVertices);
  intT* FrontierNext = newA(intT,numEdges);
  intT* Counts = newA(intT,numVertices);
  intT* Parents = newA(intT,numVertices); //parent pointers of BFS tree
  intT* NextParents = newA(intT,numVertices);
  {parallel_for(intT i = 0; i < numVertices; i++) {
      Counts[i] = 0;
      Parents[i] = -1; //unvisited nodes retain -1 value
      NextParents[i] = -1;
    }
  }
  Frontier[0] = start;
  Parents[start] = NextParents[start] = start; //parent of root is itself
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
	  if(Parents[i] != NextParents[i]) Counts[i] = 1;
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
	  for (intT j=0; j < G[v].degree; j++) {
	    intT ngh = G[v].Neighbors[j];
	    if (Parents[ngh] == -1 && utils::CAS(&Parents[ngh],-1,v)) 
	      {
		FrontierNext[o+j] = ngh; 
	      }
	    else FrontierNext[o+j] = -1;

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
      //cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n";

      parallel_for(intT i=0;i<numVertices;i++){
	NextParents[i] = Parents[i];
	if(Parents[i] == -1){
	  for(intT j=0;j<G[i].degree;j++){
	    intT ngh = G[i].Neighbors[j];
	    if(Parents[ngh] != -1) {
	      NextParents[i] = ngh;
	      break;
	    }
	  }
	}
      }

      frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Parents,NextParents));
      
      frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(), getFrontierSize(G,Parents,NextParents));
      
      if(frontierSize+frontierEdgeCount < threshold) fromSMV=1;
      swap(Parents,NextParents);

 
    }
  }
  free(FrontierNext); free(Frontier); free(Counts); 

  free(NextParents);
  
  return Parents;
}




intT* BFS2(intT start, graph<intT> GA) {
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
	  if(Visited[i] != NextVisited[i]) Counts[i] = 1;
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
	  for (intT j=0; j < G[v].degree; j++) {
	    intT ngh = G[v].Neighbors[j];
	    if (Visited[ngh] == 0 && utils::CAS(&Visited[ngh],0,1)) 
	      {
		FrontierNext[o+j] = ngh; 
		Parents[ngh] = v;
	      }
	    else FrontierNext[o+j] = -1;
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
      //cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n";

      parallel_for(intT i=0;i<numVertices;i++){
	NextVisited[i] = Visited[i];
	if(!Visited[i]){
	  for(intT j=0;j<G[i].degree;j++){
	    intT ngh = G[i].Neighbors[j];
	    if(Visited[ngh]) {
	      Parents[i] = ngh;
	      NextVisited[i] = 1;
	      break;
	    }
	  }
	}
      }

      frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,NextVisited));
      
      frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(), getFrontierSize(G,Visited,NextVisited));
      
      if(frontierSize+frontierEdgeCount < threshold) fromSMV=1;
      swap(Visited,NextVisited);

 
    }
  }
  free(FrontierNext); free(Frontier); free(Counts); 
  free(Visited); 

  free(NextVisited);
  
  return Parents;
}

intT* BFS_sparse(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Frontier = newA(intT,numEdges);
  intT* Visited = newA(intT,numVertices);
  intT* FrontierNext = newA(intT,numEdges);
  intT* Counts = newA(intT,numVertices);
  {parallel_for(intT i = 0; i < numVertices; i++) Visited[i] = -1;}

  Frontier[0] = start;
  intT frontierSize = 1;
  Visited[start] = start;

  intT totalVisited = 0;
  int round = 0;

  while (frontierSize > 0) {
    round++;
    totalVisited += frontierSize;

    {parallel_for (intT i=0; i < frontierSize; i++) 
	Counts[i] = G[Frontier[i]].degree;}
    intT nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);

    // For each vertexB in the frontier try to "hook" unvisited neighbors.
    {parallel_for(intT i = 0; i < frontierSize; i++) {
      intT k= 0;
      intT v = Frontier[i];
      intT o = Counts[i];
      for (intT j=0; j < G[v].degree; j++) {
        intT ngh = G[v].Neighbors[j];
	if (Visited[ngh] == -1 && utils::CAS(&Visited[ngh],-1,v)) {
	  FrontierNext[o+j] = ngh;
	}
	else FrontierNext[o+j] = -1;
      }
      }}

    // Filter out the empty slots (marked with -1)
    frontierSize = sequence::filter(FrontierNext,Frontier,nr,nonNegF());
  }

  free(FrontierNext); free(Frontier); free(Counts);
  return Visited;
}



void levels(intT start, intT* L, graph<intT> G){
  intT* Q = newA(intT,G.n);
  intT start_ptr = 0, end_ptr = 0;
  Q[end_ptr++] = start;
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
	}
      }
    }
    if(end_ptr == next_end_ptr) break;
    start_ptr = end_ptr; 
    end_ptr = next_end_ptr;
  }
  free(Q);
}


// Checks if T is valid BFS tree relative to G starting at i
void checkBFS(intT start, graph<intT> G, intT* Parents) {

  intT* L = newA(intT, G.n);
  parallel_for (intT i=0; i < G.n; i++) {
    L[i] = -1;
  }
  levels(start,L,G);

  if(Parents[start] != start) {
    cout<<"Parent of root not itself\n";
    abort();
  }
  parallel_for(intT i=0;i<G.n;i++) {
    if(i != start && L[i] != -1){
      if(L[Parents[i]] != L[i]-1){
	cout<<"Level of "<<i<<" wrong\n";
	abort();
      }
    }
  }
  free(L);
 
}


pair<intT,intT> MVGraph(intT start, graph<intT> GA){
  startTime();
  intT* P1 = BFS(start,GA);
  nextTime("hybrid 1");
  intT* P2 = BFS2(start,GA);
  nextTime("hybrid 2");
  intT* P3 = BFS_sparse(start,GA);
  nextTime("sparse");

  checkBFS(start,GA,P1); free(P1);
  checkBFS(start,GA,P2); free(P2);
  checkBFS(start,GA,P3); free(P3);

  return make_pair(0,0);
}
