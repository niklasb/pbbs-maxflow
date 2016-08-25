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
#include <limits.h>
#include "sequence.h"
#include "parallel.h"
#include "graph.h"
#include "ST.h"
#include "unionFind.h"
#include "deterministicHash.h"

typedef pair<intT,intT> intPair;

template <class HASH>
intT speculative_for(edge<intT>* E, intT m, intT n, int granularity, unionFind UF, HASH f) {
  intT maxTries = 100 + 200*granularity;
  intT maxRoundSize = m/granularity+1;
  Table<HASH,intT> T(maxRoundSize*2, f,2);

  intT *I = newA(intT,maxRoundSize);
  intT *Ihold = newA(intT,maxRoundSize);
  bool *keep = newA(bool,maxRoundSize);

  int round = 0; 
  intT numberDone = 0; // number of iterations done
  intT numberKeep = 0; // number of iterations to carry to next round
  intT totalProcessed = 0;

  while (numberDone < m) {
    //cout << "numberDone=" << numberDone << endl;
    if (round++ > maxTries) {
      cout << "speculativeLoop: too many iterations, increase maxTries parameter" << endl;
      abort();
    }
    intT size = min(maxRoundSize, m - numberDone);
    totalProcessed += size;
 
    //reserve
    {parallel_for(intT i=0;i<size;i++) {
	if (i >= numberKeep) I[i] = numberDone + i;
	T.insert(intPair(E[I[i]].u,i));
	T.insert(intPair(E[I[i]].v,i));
      }}
 
    //check
    {parallel_for(intT i=0;i<size;i++) {
	intT u = E[I[i]].u;
	intT v = E[I[i]].v;
      intPair uPair = T.find(u);
      intPair vPair = T.find(v);
      if(uPair.second == i) { UF.link(u,v); keep[i] = 0; }
      else if(vPair.second == i) { UF.link(v,u); keep[i] = 0; }
      else keep[i] = 1;
    }}

    //reset
    {parallel_for(intT i=0;i<size;i++) {
    	if(!keep[i]) { T.deleteVal(E[I[i]].u); T.deleteVal(E[I[i]].v); }
      }}
    //T.clear();
    
    // keep edges that failed to hook for next round
    numberKeep = sequence::pack(I, Ihold, keep, size);
    swap(I, Ihold);
    numberDone += size - numberKeep;

  }
  free(I); free(Ihold); free(keep); 
  T.del();
  return totalProcessed;
}

struct notMax { bool operator() (intT i) {return i < INT_T_MAX;}};

pair<intT*,intT> st(edgeArray<intT> G){
  intT m = G.nonZeros;
  intT n = G.numRows;
  unionFind UF(n);
  speculative_for(G.E,m,n,50,UF,hashSimplePair());
  //_seq<intT> stIdx = sequence::filter((intT*) R, n, notMax());
  //cout << "Tree size = " << stIdx.n << endl;
  UF.del(); //delete[] R;
  return make_pair((intT*)NULL,0);
  //return pair<intT*,intT>(stIdx.A, stIdx.n);
}
