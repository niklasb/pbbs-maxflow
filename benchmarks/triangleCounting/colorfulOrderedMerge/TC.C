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

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "sampleSort.h"
#endif

using namespace std;

struct isSameColor{
  isSameColor(uint mycolor, uint *colors) : colors_(colors), me_(mycolor) {};
  bool operator () (uint v) {return me_ == colors_[v];};
  uint me_;
  uint *colors_;
};

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

long countCommon(uint *A, uintT nA, uint *B, uintT nB)
{
  uintT i=0,j=0;
  long ans=0;
  while (i < nA && j < nB) {
    if (A[i]==B[j]) i++, j++, ans++;
    else if (A[i] < B[j]) i++;
    else j++;
  }
  return ans;
}

struct countFromA {
  
  countFromA (uint *_e, uint *_sz, uintT *_start)
    : edges(_e), sz(_sz), start(_start) {} ;

  long operator() (uintT _a) {
    long count = 0;
    uintT a = _a;
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
            edges + start[b], sz[b]);
      }
      count = sequence::plusReduce(cc, nn);
      free(cc);
    }
    else {
      for (uintT bi=0;bi<sz[a];bi++) {
	uintT b=edges[start[a]+bi];
	count += countCommon(edges + start[a], sz[a],
			     edges + start[b], sz[b]);
      }
    }
  
   return count;
  }
  uint* edges, *sz;
  uintT *start;
};

timer t0,t1,t2,t3,t4,t5,t6;

long countTriangle(graphC<uintT,uint> GG, double p, long seed)
{
  t0.start();
  long n = GG.n, m = GG.m;
  //sample
  uintT numColors = max<uintT>(1,1/p);
  uint* colors = newA(uint,n);
  //assign colors
  parallel_for(long i=0;i<n;i++) colors[i] = utils::hash(seed+i) % numColors;
  t0.reportTotal("assign colors");
  t5.start();
  uint* edgesSparse = newA(uint,m);
  //keep only neighbors matching own color
  uintT* Degrees = newA(uintT,n+1); Degrees[n] = 0;
  parallel_for(long i=0;i<n;i++) {
    uintT start = GG.offsets[i];
    uintT d = GG.offsets[i+1]-start;
    uint color = colors[i];
    if(d > 10000) {
      Degrees[i] = sequence::filter(GG.edges+start, edgesSparse+start, 
    			       d, isSameColor(color, colors));
    } else {
      uintT k = 0;
      uint* newNghs = edgesSparse + start;
      uint* Nghs = GG.edges + start;
      for(uintT j=0;j<d;j++) {
	uintT ngh = Nghs[j];
	if(colors[ngh] == color) newNghs[k++] = ngh;
      }
      Degrees[i] = k;
    }
  }
  free(colors);
  t5.reportTotal("subsample edges");
  t6.start();
  long new_m = sequence::plusScan(Degrees,Degrees,n+1);
  //cout << "m = " << m << " new_m = " << new_m<<endl;

  //creating subgraph
  uint* new_edges = newA(uint,new_m);
  parallel_for(long i=0;i<n;i++) {
    uintT start = GG.offsets[i];
    uint* Nghs = edgesSparse + start;
    uintT o = Degrees[i];
    uintT d = Degrees[i+1]-o;
    if(d < 10000) {
      for(uintT j=0;j<d;j++) {
  	new_edges[o+j] = Nghs[j];
      }
    } else {
      parallel_for(uintT j=0;j<d;j++) {
  	new_edges[o+j] = Nghs[j];
      }
    }
  }
  free(edgesSparse);
  graphC<uintT,uint> G(Degrees,new_edges,n,new_m);
  t6.reportTotal("create subgraph");

  t1.start();
  uint *rank = newA(uint, n);
  uint *order = newA(uint, n);

  rankNodes(G, rank, order);
  t1.reportTotal("ranking");

  // create a directed version of G and order the nodes in
  // the increasing order of rank

  m = new_m;
  uint *edges = newA(uint, m);  
  uint *sz = order;


  t2.start();  
  parallel_for (long s=0;s<G.n;s++) {
    uintT o = Degrees[s];
    uintT d = Degrees[s+1]-o;
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

  // start counting
  t3.start();
  long count = 
    sequence::reduce<long>((long) 0, (long) n, 
			   utils::addF<long>(),
			   countFromA(edges,sz,Degrees));
  G.del();
  free(edges); free(sz);
  t3.reportTotal("counting");
  long triCount = (p == 1.0) ? count : (long) ((double)count/(p*p));
  cout << "tri. count = " << triCount << endl;
  return triCount;
}
