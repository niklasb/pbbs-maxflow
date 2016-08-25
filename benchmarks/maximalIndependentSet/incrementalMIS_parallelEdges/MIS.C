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

#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "speculative_for.h"
#include "gettime.h"
using namespace std;

// **************************************************************
//    MAXIMAL INDEPENDENT SET
// **************************************************************

// For each vertex:
//   Flags = 0 indicates a neighbor is chosen
//   Flags = 1 indicates undecided
//   Flags = 2 indicates vertex is chosen
struct MISstep {
  intT flag;
  intT *Flags;  vertex<intT>*G;
  MISstep(intT* _F, vertex<intT>* _G) : Flags(_F), G(_G) {}

  bool reserve(intT i) {
    intT d = G[i].degree;
    flag = 2;
    intT curr = 0;

    for(; curr<min(64,d);curr++){
      intT ngh=G[i].Neighbors[curr];
      if(ngh<i){
	if(Flags[ngh]==2){flag=0; return 1;}
	else if(Flags[ngh]==1) flag=1;
      }
    }

    intT k=8;
    while(d-curr>0){
      intT size = min(1<<k++,d-curr);
      parallel_for(intT j=0;j<size;j++){
	intT ngh = G[i].Neighbors[curr+j];
	if(ngh < i){
	  if(Flags[ngh] == 2) {utils::writeMin(&flag,0);}
	  else if (Flags[ngh] == 1) utils::writeMin(&flag,1);
	}
      }
      if(flag == 0) return 1;
      curr+=size;
    }
    return 1;
  }

  bool commit(intT i) { 
    return (Flags[i] = flag) != 1;
  }
};

char* maximalIndependentSet(graph<intT> GS) {
  intT n = GS.n;
  vertex<intT>* G = GS.V;
  intT* Flags = newArray(n, 1);
  MISstep mis(Flags, G);
  speculative_for(mis,0,n,20);
  char* Flagsc = newArray(n,(char)0);
  parallel_for(intT i=0;i<n;i++) {
    if(Flags[i] == 2) Flagsc[i] = 1;
    else if(Flags[i] == 0) Flagsc[i] = 2;
  }
  free(Flags);
  return Flagsc;
}
