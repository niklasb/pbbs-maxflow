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
#include <algorithm>
#include <set>
#include <cstring>
#include <cassert>
#include "sequence.h"
#include "graph.h"
#include "utils.h"
#include "parallel.h"
#include "gettime.h"
#include "intHash.h"

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "sampleSort.h"
#endif

using namespace std;

// **************************************************************
// * using the degree heuristic to order the vertices
// **************************************************************

typedef ETable<hashInt<uint>, uintT> intTable;

struct nodeLT {
  nodeLT(graphC<uintT,uint> G_) : G(G_) {};

  bool operator() (uint a, uint b) {
    uintT deg_a = G.offsets[a+1]-G.offsets[a];
    uintT deg_b = G.offsets[b+1]-G.offsets[b];
    return deg_a < deg_b;
  };  
  graphC<uintT,uint> G;
};

void rankNodes(graphC<uintT,uint> G, uint *r, uint *o)
{
  parallel_for (long i=0;i<G.n;i++) o[i] = i;
  compSort(o, G.n, nodeLT(G));
  parallel_for (long i=0;i<G.n;i++) r[o[i]] = i;  
}

struct isFwd{
  isFwd(uint myrank, uint *r) : r_(r), me_(myrank) {};
  bool operator () (uint v) {return r_[v] > me_;};
  uint me_;
  uint *r_;
};

struct intLT {
  bool operator () (uintT a, uintT b) { return a < b; };
};

inline long countCommon(uintT a, uint *A, uintT nA, uintT b, uint *B, uintT nB, 
		 intTable &ta, intTable* TA)
{
  long ans = 0;
  if(nA < nB) {
    intTable T = TA[b];
    for(uintT i=0;i<nA;i++)
      if(T.find(A[i])) ans++;
  } else {
    for(uintT i=0;i<nB;i++)
      if(ta.find(B[i])) ans++;
  }
  return ans;
}

struct countFromA {
  countFromA (uint *_e, uint *_sz, uintT *_start, 
	      intTable* _TA)
    : edges(_e), sz(_sz), start(_start), TA(_TA) {} ;

  long operator() (uintT _a) {
    long count = 0;
    uintT a = _a;
    uintT tw = 0;
    for (uintT bi=0;bi<sz[a];bi++) {
      uintT b=edges[start[a]+bi];
      tw += min<uintT>(sz[a], sz[b]);
    }
    intTable ta = TA[a];
    if (tw > 10000) {
      long *cc = newA(long, sz[a]);
      uintT nn = sz[a];
      parallel_for (uintT bi=0;bi<nn;bi++) {
        uintT b=edges[start[a]+bi];
        cc[bi] = countCommon(a, edges + start[a], sz[a],
			     b, edges + start[b], sz[b], ta, TA);
      }
      count = sequence::plusReduce(cc, nn);
      free(cc);
    }
    else {
      for (uintT bi=0;bi<sz[a];bi++) {
	uintT b=edges[start[a]+bi];
	count += countCommon(a, edges + start[a], sz[a],
			     b, edges + start[b], sz[b], ta, TA);
      }
    }  
   return count;
  }
  uint* edges,*sz;
  uintT *start;
  intTable* TA;
};

timer t1,t2,t3,t4,t5;

long countTriangle(graphC<uintT,uint> G, double p, long seed)
{
  t1.start();
  uintT n = G.n;
  uint *rank = newA(uint, n);
  uint *order = newA(uint, n);

  rankNodes(G, rank, order);
  t1.reportTotal("ranking");

  // create a directed version of G and order the nodes in
  // the increasing order of rank

  uint *edges = newA(uint, G.m);
  uint *sz = order;
  
  t2.start();
  
  parallel_for (uintT s=0;s<n;s++) {
    uintT o = G.offsets[s];
    uintT d = G.offsets[s+1]-o;
    if(d > 10000) {
      sz[s] = sequence::filter(G.edges+o, edges + o, 
    			       d, isFwd(rank[s], rank));
    } else {
      uintT k=0;
      uint* newNgh = edges+o;
      uint* Ngh = G.edges+o;
      for(uintT j=0;j<d;j++) { 
	uintT ngh = Ngh[j];
	if(rank[s] < rank[ngh]) newNgh[k++] = ngh;
      }
      sz[s] = k;
    }
    //compSort(edges+o, sz[s], intLT());
  }
  free(rank); 
  t2.reportTotal("filtering");

  t4.start();
  //compute hash table offsets
  uintT* hoffsets = newA(uintT,n+1);
  parallel_for(uintT i=0;i<n;i++) hoffsets[i] = 1 << (utils::log2Up((uintT)(1+sz[i]))); 
  //hash table of 2x the size
  hoffsets[n] = 0;
  uintT totalSize = sequence::plusScan(hoffsets,hoffsets,n+1);
  //cout << G.m/2 << " " << totalSize << " " << totalSize*sizeof(uint) << " bytes requested\n";
  uint* A = newA(uint,totalSize);

  intTable* TA = newA(intTable,n);
  parallel_for(long s=0;s<n;s++) {
    uintT size = hoffsets[s+1]-hoffsets[s];
    TA[s] = intTable(size, hashInt<uint>(), A+hoffsets[s]);
  }
  free(hoffsets);

  //everyone inserts edges
  parallel_for (uintT s=0;s<n;s++) {
    uintT o = G.offsets[s];
    uintT d = G.offsets[s+1]-o;
    uint* Ngh = edges+o;
    if(d > 10000)
      parallel_for(uintT j=0;j<sz[s];j++) TA[s].insert(Ngh[j]);
    else 
      for(uintT j=0;j<sz[s];j++) TA[s].insert(Ngh[j]);
  }
  t4.reportTotal("insert edges into hash table");

  // start counting
  t3.start();

  long count = 
    sequence::reduce<long>((long) 0, (long) n, 
			   utils::addF<long>(),
			   countFromA(edges,sz,G.offsets,TA));
  free(TA); free(A);
  free(edges); free(sz);
  t3.reportTotal("counting");

  cout << "tri. count = " << count << endl;
  return count;
}
