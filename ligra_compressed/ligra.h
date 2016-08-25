// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
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

#ifndef LIGRA_H
#define LIGRA_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <algorithm>
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "IO.h"
#include "bytecode.h"
using namespace std;

//*****START FRAMEWORK*****

//*****VERTEX OBJECT*****
struct vertices {
  long n, m;
  uintE* s;
  bool* d;
  bool isDense;
  // make a singleton vertex in range of n
vertices(long _n, intE v) 
: n(_n), m(1), d(NULL), isDense(0) {
  s = newA(uintE,1);
  s[0] = v;
}  
  //empty vertex set
vertices(long _n) : n(_n), m(0), d(NULL), s(NULL), isDense(0) {}

  // make vertices from array of vertex indices
  // n is range, and m is size of array
vertices(long _n, long _m, uintE* indices) 
: n(_n), m(_m), s(indices), d(NULL), isDense(0) {}
  // make vertices from boolean array, where n is range
vertices(long _n, bool* bits) 
: n(_n), d(bits), s(NULL), isDense(1)  {
  m = sequence::sum(bits,_n); }
  // make vertices from boolean array giving number of true values
vertices(long _n, long _m, bool* bits) 
: n(_n), m(_m), s(NULL), d(bits), isDense(1)  {}

  // delete the contents
  void del(){
    if (d != NULL) free(d);
    if (s != NULL) free(s);
  }
  long numRows() { return n; }
  long numNonzeros() { return m; }
  bool isEmpty() { return m==0; }

  // converts to dense but keeps sparse representation if there
  void toDense() {
    if (d == NULL) {
      d = newA(bool,n);
      {parallel_for(long i=0;i<n;i++) d[i] = 0;}
      {parallel_for(long i=0;i<m;i++) d[s[i]] = 1;}
    }
    isDense = true;
  }
  // converts to sparse but keeps dense representation if there
  void toSparse() {
    if (s == NULL) {
      _seq<uintE> R = sequence::packIndex<uintE>(d,n);
      if (m != R.n) {
	cout << "bad stored value of m" << endl; 
	abort();
      }
      s = R.A;
    }
    isDense = false;
  }
  // check for equality
  bool eq (vertices& b) {
    toDense();
    b.toDense();
    bool* c = newA(bool,n);
    {parallel_for (long i=0; i<b.n; i++) 
	c[i] = (d[i] != b.d[i]);}
    bool equal = (0 == sequence::sum(c,n));
    free(c);
    return equal;
  }
  void print() {
    if (isDense) {
      cout << "D: ";
      for (long i=0;i<n;i++) if (d[i]) cout << i << " ";
      cout << endl;
    } else {
      cout << "S: ";
      for (long i=0; i<m; i++)	cout << s[i] << " ";
      cout << endl;
    }
  }
};

struct nonMaxF{bool operator() (const uintE &a) {return (a != UINT_E_MAX);}};

//options to edgeMap for different versions of dense edgeMap (default is DENSE)
enum options { DENSE, DENSE_FORWARD };

//remove duplicate integers in [0,...,n-1]
void remDuplicates(uintE* indices, uintE* flags, long m, long n) {
  //make flags for first time
  if(flags == NULL) {flags = newA(uintE,n); 
    {parallel_for(long i=0;i<n;i++) flags[i]=UINT_E_MAX;}}
  {parallel_for(uintE i=0;i<m;i++)
      if(indices[i] != UINT_E_MAX && flags[indices[i]] == UINT_E_MAX) 
	CAS(&flags[indices[i]],(uintE)UINT_E_MAX,i);
  }
  //reset flags
  {parallel_for(long i=0;i<m;i++){
      if(indices[i] != UINT_E_MAX){
	if(flags[indices[i]] == i){ //win
	  flags[indices[i]] = UINT_E_MAX; //reset
	}
	else indices[i] = UINT_E_MAX; //lost
      }
    }
  }
}

//*****EDGE FUNCTIONS*****

template <class F>
struct denseT {
  bool* nextArr, *vertexArr;
denseT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
    if (vertexArr[target] && f.update(target, src)) nextArr[src] = 1;
    return f.cond(src);
  }
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
    if (vertexArr[target] && f.update(target, src, weight)) nextArr[src] = 1;
    return f.cond(src);
  }};

template <class F, class vertex>
  bool* edgeMapDense(graph<vertex> GA, bool* vertices, F f) {  
  long numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  parallel_for (long i=0; i<numVertices; i++){
    next[i] = 0;
    if (f.cond(i)) { 
//      intT d = G[i].getInDegree();
      char *nghArr = G[i].getInNeighbors();
#ifdef WEIGHTED
      decodeWgh(denseT<F>(next, vertices), f, nghArr, i, G[i].getInDegree());
#else
      decode(denseT<F>(next, vertices), f, nghArr, i, G[i].getInDegree());
#endif
    }
  }
  return next;
}

template <class F>
struct denseForwardT {
  bool* nextArr, *vertexArr;
denseForwardT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
    if (f.cond(target) && f.updateAtomic(src,target)) nextArr[target] = 1;
    return true;
  }
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
    if (f.cond(target) && f.updateAtomic(src,target, weight)) nextArr[target] = 1;
    return true;
  }};

template <class F, class vertex>
  bool* edgeMapDenseForward(graph<vertex> GA, bool* vertices, F f) {
  long numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  {parallel_for(long i=0;i<numVertices;i++) next[i] = 0;}
  {parallel_for (long i=0; i<numVertices; i++) {
//    intT d = G[i].getOutDegree();
    char *nghArr = G[i].getOutNeighbors();
#ifdef WEIGHTED
    decodeWgh(denseForwardT<F>(next, vertices), f, nghArr, i, G[i].getOutDegree());
#else
    decode(denseForwardT<F>(next, vertices), f, nghArr, i, G[i].getOutDegree());
#endif
  }}
  return next;
}

template <class F>
struct sparseT {
  uintT v, o;
  uintE *outEdges;
sparseT(uintT vP, uintT oP, uintE *outEdgesP) : v(vP), o(oP), outEdges(outEdgesP) {}
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
    if (f.cond(target) && f.updateAtomic(v, target))
      outEdges[o + edgeNumber] = target;
    else outEdges[o + edgeNumber] = UINT_E_MAX;
    return true; }
  inline bool srcTarg(F f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
    if (f.cond(target) && f.updateAtomic(v, target, weight))
      outEdges[o + edgeNumber] = target;
    else outEdges[o + edgeNumber] = UINT_E_MAX;
    return true; }};



template <class F, class vertex>
pair<long,uintE*> edgeMapSparse(vertex* frontierVertices, uintE* indices, 
			       uintT* degrees, long m, F f,
				long remDups=0, uintE* flags=NULL) {
  uintT* offsets = degrees;
  long outEdgeCount = sequence::plusScan(offsets, degrees, m);
  uintE* outEdges = newA(uintE,outEdgeCount);
  // In parallel, for each edge in frontier vertices, get out-degree and check
  parallel_for (long i = 0; i < m; i++) {
    uintT v = indices[i], o = offsets[i];
    vertex vert = frontierVertices[i]; 
//    intT d = vert.getOutDegree();
    char *nghArr = vert.getOutNeighbors();
    // Decode, with src = v, and degree d, applying sparseT
#ifdef WEIGHTED
    decodeWgh(sparseT<F>(v, o, outEdges), f, nghArr, v, vert.getOutDegree());
#else
    decode(sparseT<F>(v, o, outEdges), f, nghArr, v, vert.getOutDegree());
#endif
  }
  uintE* nextIndices = newA(uintE, outEdgeCount);
  if(remDups) remDuplicates(outEdges,flags,outEdgeCount,remDups);
  // Filter out the empty slots (marked with UINT_E_MAX)
  long nextM = sequence::filter(outEdges,nextIndices,outEdgeCount,nonMaxF());
  free(outEdges);
  return pair<long,uintE*>(nextM, nextIndices);
}

static long edgesTraversed = 0;

// decides on sparse or dense base on number of nonzeros in the active vertices
template <class F, class vertex>
  vertices edgeMap(graph<vertex> GA, vertices &V, F f, long threshold = -1, 
		 char option=DENSE, bool remDups=false) {
  long numVertices = GA.n, numEdges = GA.m;
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = GA.V;
  long m = V.numNonzeros();
  if (numVertices != V.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }
  // used to generate nonzero indices to get degrees
  uintT* degrees = newA(uintT, m);
  vertex* frontierVertices;
  V.toSparse();
  frontierVertices = newA(vertex,m);
  {parallel_for (long i=0; i < m; i++){
    vertex v = G[V.s[i]];
    degrees[i] = v.getOutDegree();
    frontierVertices[i] = v;
    }}
  uintT outDegrees = sequence::plusReduce(degrees, m);
  edgesTraversed += outDegrees;
  if (outDegrees == 0) return vertices(numVertices);
  if (m + outDegrees > threshold) { 
    V.toDense();
    free(degrees);
    free(frontierVertices);
    bool* R = (option == DENSE_FORWARD) ? 
      edgeMapDenseForward(GA,V.d,f) : 
      edgeMapDense(GA, V.d, f);
    vertices v1 = vertices(numVertices, R);
    return  v1;
  } else { 
    pair<long,uintE*> R = 
      remDups ? 
      edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), f,numVertices, GA.flags) :
      edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), f);
    //cout << "size (S) = " << R.first << endl;
    free(degrees);
    free(frontierVertices);
    return vertices(numVertices, R.first, R.second);
  }
}

//*****VERTEX FUNCTIONS*****

template <class F>
void vertexMap(vertices V, F add) {
  long n = V.numRows(), m = V.numNonzeros();
  if(V.isDense) {
    {parallel_for(long i=0;i<n;i++)
	if(V.d[i]) add(i);}
  } else {
    {parallel_for(long i=0;i<m;i++)
	add(V.s[i]);}
  }
}

template <class F>
vertices vertexFilter(vertices V, F filter) {
  long n = V.numRows(), m = V.numNonzeros();
  V.toDense();
  bool* d_out = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) d_out[i] = 0;}
  {parallel_for(long i=0;i<n;i++)
      if(V.d[i]) d_out[i] = filter(i);}
  return vertices(n,d_out);
}

//Cond function that always returns true
inline bool cond_true (const uintE &d) { return 1;}

#endif
