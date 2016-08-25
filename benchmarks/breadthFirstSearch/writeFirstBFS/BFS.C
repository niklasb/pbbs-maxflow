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
#include <limits.h>
#include "gettime.h"
using namespace std;


timer t1,t2,t3,t4;

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

template <class ET>
struct pairCmp {
  bool operator() (pair<ET,ET> p1, pair<ET,ET> p2) {
     return (p1.first < p2.first);
  }
};

typedef pair<intT,intT> intPair;

pair<intT,intT> BFS(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Offsets = newA(intT,numVertices+1);
  intPair* indexParents = newA(intPair,numVertices);
  //intT* Parents = newA(intT,numVertices);
  parallel_for (intT i = 0; i < numVertices; i++) indexParents[i] = make_pair(INT_T_MAX,INT_T_MAX);
  intT* Frontier = newA(intT,numVertices);
  intT* FrontierNext = newA(intT,numEdges);

  Frontier[0] = start;
  intT fSize = 1;
  indexParents[start] = make_pair(-1,start);
  //Parents[start] = -start;
  int round = 0;
  intT totalVisited = 0;

  //vertex<intT>* frontierVertices = newA(vertex<intT>,numVertices);
 
  while (fSize > 0) {
    round++;
    t1.start();
    // For each vertex in the frontier try to "hook" unvisited neighbors.
    parallel_for(intT i = 0; i < fSize; i++) {
      intT v = Frontier[i];
      vertex<intT> vert = G[v];
      Offsets[i] = vert.degree;
      //frontierVertices[i] = vert;
      if(vert.degree > 1000){
	parallel_for (intT j=0; j < vert.degree; j++) {
	  intT ngh = vert.Neighbors[j];
	  intT val = i+totalVisited;
	  if (indexParents[ngh].first > val)
	    utils::writeMin(&indexParents[ngh],make_pair(val,v),pairCmp<intT>());
	}
      } 
      else{
	for (intT j=0; j < vert.degree; j++) {
	  intT ngh = vert.Neighbors[j];
	  intT val = i+totalVisited;
	  if (indexParents[ngh].first > val)
	      utils::writeMin(&indexParents[ngh],make_pair(val,v),pairCmp<intT>());
	}
      } 
    }
    t1.stop();
    // Find offsets to write the next frontier for each v in this frontier
    intT nr=sequence::scan(Offsets,Offsets,fSize,utils::addF<intT>(),(intT)0);
    Offsets[fSize] = nr;
    
    t3.start();

    // Move hooked neighbors to next frontier.   
    parallel_for (intT i = 0; i < fSize; i++) {
      intT o = Offsets[i];
      intT v = Frontier[i];
      vertex<intT> vert = G[v];//frontierVertices[i];
      if(vert.degree > 1000){
    	parallel_for (intT j=0; j < vert.degree; j++) {
    	  intT ngh = vert.Neighbors[j];
    	  intT val = i+totalVisited;
    	  if (indexParents[ngh].first == val) {
    	    FrontierNext[o+j] = ngh;
    	    //indexParents[ngh].first = -1;
    	    //Parents[ngh] = -v-1;
    	  }
    	  else FrontierNext[o+j] = -1;
    	}
      }
      else {
    	for (intT j=0; j < vert.degree; j++) {
    	  intT ngh = vert.Neighbors[j];
    	  intT val = i+totalVisited;
    	  if (indexParents[ngh].first == val) {
    	    FrontierNext[o+j] = ngh;
    	    //indexParents[ngh].first = -1;
    	    //Parents[ngh] = -v-1;
    	  }
    	  else FrontierNext[o+j] = -1;
    	}
      }
    }
    t3.stop();
    totalVisited += fSize;

    t4.start();
    // Filter out the empty slots (marked with -1)
    fSize = sequence::filter(FrontierNext, Frontier, nr, nonNegF());
    t4.stop();

  }

  //free(frontierVertices);
  free(FrontierNext); free(Frontier); free(Offsets); free(indexParents);

  // cout<<"hook:";t1.reportTotal();
  // cout<<"scan:";t2.reportTotal();
  // cout<<"move to frontier:";t3.reportTotal();
  // cout<<"filter:";t4.reportTotal();

  return pair<intT,intT>(totalVisited,round);
}
