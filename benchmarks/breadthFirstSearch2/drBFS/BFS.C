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
#include <limits.h>
#include "speculative_for.h"
using namespace std;

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

struct BFSstep {
  intT* Frontier, *FrontierNext, *backPtrs, *Offsets;
  vertex<intT>* G;
  BFSstep(vertex<intT>* _G, intT* _F, intT* _FN, intT* _b, intT* _O) : 
    G(_G), Frontier(_F), FrontierNext(_FN), backPtrs(_b), Offsets(_O) {}

  bool reserve(intT i) {
    intT k = 0;
    intT v = Frontier[i];
    for (intT j=0; j < G[v].degree; j++) {
      intT ngh = G[v].Neighbors[j];
      if (backPtrs[ngh] > v)
	if(utils::writeMin(&backPtrs[ngh],v))
	  G[v].Neighbors[k++] = ngh;
    }
    G[v].degree = k;
    return 1;
  }

  bool commit(intT i) { 
    intT o = Offsets[i];
    intT v = Frontier[i];
    intT d = G[v].degree;
    for (intT j=0; j < d; j++) {
      intT ngh = G[v].Neighbors[j];
      if (backPtrs[ngh] == v) {
	FrontierNext[o+j] = ngh;
	backPtrs[ngh] = -v;
      }
      //else FrontierNext[o+j] = -1;
    }
    return 1;
  }
};

pair<intT,intT*> BFS(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* Offsets = newA(intT,numVertices+1);

  intT* backPtrs = newA(intT,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) backPtrs[i] = INT_T_MAX;
  intT* Frontier = newA(intT,numVertices);
  intT* FrontierNext = newA(intT,numEdges);

  Frontier[0] = start;
  intT fSize = 1;
  backPtrs[start] = start;
  int round = 0;
  intT totalVisited = 0;

  while (fSize > 0) {
    totalVisited += fSize;
    round++;
    parallel_for(intT i=0; i<fSize; i++) {
      Offsets[i] = G[Frontier[i]].degree;
    }
    intT nr=sequence::scan(Offsets,Offsets,fSize,utils::addF<intT>(),(intT)0);
    parallel_for(intT i = 0; i<nr;i++) FrontierNext[i] = -1;
    speculative_for(BFSstep(G,Frontier,FrontierNext,backPtrs, Offsets),
		    0, fSize, 1, 0);

    // Filter out the empty slots (marked with -1)
    fSize = sequence::filter(FrontierNext, Frontier, nr, nonNegF());
  }

  parallel_for(intT i=0;i<numVertices;i++) {
    if(backPtrs[i] == INT_T_MAX) backPtrs[i] = -1;
    else backPtrs[i] = -backPtrs[i];
  }

  free(FrontierNext); free(Frontier); free(Offsets);

  return pair<intT,intT*>(totalVisited,backPtrs);
}
