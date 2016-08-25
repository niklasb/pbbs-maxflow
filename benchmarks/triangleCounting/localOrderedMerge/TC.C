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
#include "blockRadixSort.h"
#include <fstream>

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

long countCommon(uint *A, uintT nA, uint *B, uintT nB, long* &localCounts)
{
  uintT i=0,j=0;
  long ans=0;
  while (i < nA && j < nB) {
    if (A[i]==B[j]) { utils::xaddl(&localCounts[A[i]], (long) 1); i++, j++, ans++;}
    else if (A[i] < B[j]) i++;
    else j++;
  }
  return ans;
}


void countFromA(uintT a, uint *edges, uint *sz, uintT *start, long* localCounts)
{
  long count = 0;
  uintT tw = 0;
  for (uintT bi=0;bi<sz[a];bi++) {
    uintT b=edges[start[a]+bi];
    tw += min<uintT>(sz[a], sz[b]);
  }
  if (tw > 10000) {
    long *cc = newA(long, sz[a]);
    uintT nn = sz[a];
    parallel_for (uintT bi=0;bi<nn;bi++) {
      uintT b=edges[start[a]+bi];
      cc[bi] = countCommon(edges + start[a], sz[a],
			   edges + start[b], sz[b], localCounts);
      if(cc[bi]) utils::xaddl(&localCounts[b],cc[bi]);
    }
    count = sequence::plusReduce(cc, nn);
    free(cc);
  }
  else {
    for (uintT bi=0;bi<sz[a];bi++) {
      uintT b=edges[start[a]+bi];
      long b_count =
	countCommon(edges + start[a], sz[a],
		    edges + start[b], sz[b], localCounts);
      if(b_count) utils::xaddl(&localCounts[b],b_count);
      count += b_count;
    }
  }
  if(count) utils::xaddl(&localCounts[a],count);  
}

struct nonMaxEdge {
  bool operator() (edge<uintT> e) {return e.u != UINT_T_MAX;}
};


edgeArray<uintT> writeCountsToFile(long* localCounts, long n, char* outFile){

  long maxV = sequence::reduce(localCounts, n, utils::maxF<long>());
  intSort::iSort(localCounts, n, maxV+1,  utils::identityF<long>());

  uintT* offsets = newA(uintT,maxV+2);
  offsets[maxV+1] = n;
  parallel_for(long i=0;i<maxV+1;i++) offsets[i] = UINT_T_MAX;
  offsets[localCounts[0]] = 0;

  parallel_for(long i=1;i<n;i++) {
    if(localCounts[i] != localCounts[i-1]) offsets[localCounts[i]] = i;
  }

  sequence::scanIBack(offsets, offsets, maxV+2, 
		     utils::minF<uintT>(), (uintT)UINT_T_MAX);

  edge<uintT>* E = newA(edge<uintT>, maxV+1);
  parallel_for(long i=0;i<maxV+1;i++) {
    if(offsets[i+1]-offsets[i] > 0) {E[i].u = i; E[i].v = offsets[i+1]-offsets[i];}
    else {E[i].u = E[i].v = UINT_T_MAX;}
  }
  free(offsets);
  edge<uintT>* E2 = newA(edge<uintT>,maxV+1);
  long Esize  = sequence::filter(E,E2,maxV+1,nonMaxEdge());
  free(E);
  return edgeArray<uintT>(E2,1,1,Esize);
  // if(outFile != NULL){
  //   writeEdgeArrayToFile(edgeArray<uintT>(E2,1,1,Esize),outFile);

    // stringstream toWrite;
    // for(long i=0;i<maxV+1;i++) {
    //   if(offsets[i+1]-offsets[i] > 0)
    // 	toWrite<<i<<" "<<offsets[i+1] - offsets[i] << endl;
   
    //   ofstream oFile(outFile);
    //   oFile<<toWrite.str();
    //   oFile.close();
    //}
  //  }
  //free(E2);

}

timer t1,t2,t3,t4,t5;

long countTriangle(graphC<uintT,uint> G, double p, long seed)
{
  t1.start();
  uint *rank = newA(uint, G.n);
  uint *order = newA(uint, G.n);

  rankNodes(G, rank, order);
  t1.reportTotal("ranking");
  // create a directed version of G and order the nodes in
  // the increasing order of rank

  uint *edges = newA(uint, G.m);
  uint *sz = order;
  t2.start();
  
  parallel_for (long s=0;s<G.n;s++) {
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
    compSort(edges+o, sz[s], intLT());
  }
  free(rank); 
  t2.reportTotal("filtering");
  free(G.edges);
  // start counting
  t3.start();
  long* localCounts = newA(long,G.n);
  parallel_for(long i=0;i<G.n;i++) localCounts[i] = 0;
  parallel_for(long i=0;i<G.n;i++)
    countFromA(i,edges,sz,G.offsets,localCounts);
  
  free(edges); free(sz);
  //cout << "local counts sum = " << sequence::plusReduce(localCounts,G.n) << endl;;
  t3.reportTotal("counting");
  //edgeArray<uintT> E = writeCountsToFile(localCounts,G.n,(char*)"a");

  free(localCounts);
  //cout << "writing to file\n";
  //cout << "tri. count = " << count << endl;
  return 1;
}
