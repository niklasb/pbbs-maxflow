// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch, Julian Shun, and the PBBS team
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
#include <stdio.h>
#include <assert.h>
#include "gettime.h"
using namespace std;

struct vertices {
  intT n, m;
  intT* s;
  bool* d;
  bool isDense;

  // make a singleton vertex in range of n
vertices(intT _n, intT v) 
: n(_n), m(1), d(NULL), isDense(0) {
  s = newA(intT,1);
  s[0] = v;
}
  //empty vertex set
vertices(intT _n) : n(_n), m(0), d(NULL), s(NULL), isDense(0) {}

  // make vertices from array of vertex indices
  // n is range, and m is size of array
vertices(intT _n, intT _m, intT* indices) 
: n(_n), m(_m), s(indices), d(NULL), isDense(0) {}

  // make vertices from boolean array, where n is range
vertices(intT _n, bool* bits) 
: n(_n), d(bits), s(NULL), isDense(1)  {
  m = sequence::sum(bits,_n);
}

  // make vertices from boolean array giving number of true values
vertices(intT _n, intT _m, bool* bits) 
: n(_n), m(_m), s(NULL), d(bits), isDense(1)  {}

  // delete the contents
  void del(){
    if (d != NULL); free(d);
    if (s != NULL); free(s);
  }

  intT numRows() { return n; }

  intT numNonzeros() { return m; }

  bool isEmpty() { return m==0; }

  // converts to dense but keeps sparse representation if there
  void toDense() {
    if (d == NULL) {
      d = newA(bool,n);
      parallel_for(intT i=0;i<n;i++) d[i] = 0;
      parallel_for(intT i=0;i<m;i++) d[s[i]] = 1;
    }
    isDense = true;
  }

  // converts to sparse but keeps dense representation if there
  void toSparse() {
    if (s == NULL) {
      _seq<intT> R = sequence::packIndex(d,n);
      if (m != R.n) {
	cout << "bad stored value of m" << endl; 
	abort();
      }
      s = R.A;
    }
    isDense = false;
  }

  // Currently always converts vertex set to dense 
  void merge (vertices& b) {
    if (n != b.n) {
      cout << "in union: add lengths don't match" << endl; 
      abort();
    }
    toDense();
    if (b.isDense) {
      parallel_for (intT i=0;i<n;i++) d[i] |= b.d[i];
    } else {
      parallel_for (intT i=0; i<b.m; i++) 
	if(!d[b.s[i]]) d[b.s[i]] = 1;
    }
    if (s != NULL) {free(s); 
      s = NULL;}
    m = sequence::reduce<intT>((intT)0,n,utils::addF<intT>(),
			       sequence::boolGetA<intT>(d));
  }

  // check for equality
  bool eq (vertices& b) {
    toDense();
    b.toDense();
    bool* c = newA(bool,n);
    parallel_for (intT i=0; i<b.n; i++) 
      c[i] = (d[i] != b.d[i]);
    return (sequence::sum(c,n) == 0);
  }

  void print() {
    if (isDense) {
      cout << "D:";
      for (intT i=0;i<n;i++) if (d[i]) cout << i << " ";
      cout << endl;
    } else {
      cout << "S:";
      for (intT i=0; i<m; i++)	cout << s[i] << " ";
      cout << endl;
    }
  }
};

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

template <class F>
bool* edgeMapDense(wghGraph<intT> GA, bool* vertices, F add) {
  intT numVertices = GA.n;
  wghVertex<intT> *G = GA.V;
  bool* next = newA(bool,numVertices);
  parallel_for (intT i=0; i<numVertices; i++){
    next[i] = 0;
    if (add.cond(i)) {
      for(intT j=0; j<G[i].degree; j++){
	intT ngh = G[i].Neighbors[j];
	if(vertices[ngh]){
	  intT edgeLen = G[i].nghWeights[j];
	  if (vertices[ngh]&&add.add(ngh,i,edgeLen)) next[i] = 1;
	}
	if (!add.cond(i)) break;
      }
    }
  }
  return next;
}

template <class F>
bool* edgeMapDenseForward(wghGraph<intT> GA, bool* vertices, F add) {
  intT numVertices = GA.n;
  wghVertex<intT> *G = GA.V;
  bool* next = newA(bool,numVertices);
  {parallel_for(intT i=0;i<numVertices;i++) next[i] = 0;}
  {parallel_for (intT i=0; i<numVertices; i++){
    if (vertices[i]) {
      for(intT j=0; j<G[i].degree; j++){
	intT ngh = G[i].Neighbors[j];
	intT edgeLen = G[i].nghWeights[j];
	if (add.cond(ngh) && add.addAtomic(i,ngh,edgeLen)) next[ngh] = 1;
      }
    }
    }}
  return next;
}

template <class F>
pair<uintT,intT*> edgeMapSparse(wghVertex<intT>* frontierVertices, intT* indices, 
			       uintT* degrees, intT m, F add) {

  uintT* offsets = degrees;
  uintT outEdgeCount = sequence::plusScan(offsets, degrees, m);

  intT* outEdges = newA(intT,outEdgeCount);

  parallel_for (intT i = 0; i < m; i++) {
    intT v = indices[i];
    intT o = offsets[i];
    wghVertex<intT> vert = frontierVertices[i];
    for (intT j=0; j < vert.degree; j++) {
      intT ngh = vert.Neighbors[j];
      intT edgeLen = vert.nghWeights[j];
      if (add.cond(ngh) && add.addAtomic(v,ngh,edgeLen)) 
	outEdges[o+j] = ngh;
      else outEdges[o+j] = -1;
    }
  }

  intT* nextIndices = newA(intT, outEdgeCount);

  // Filter out the empty slots (marked with -1)
  uintT nextM = sequence::filter(outEdges,nextIndices,outEdgeCount,nonNegF());

  free(outEdges);
  return pair<uintT,intT*>(nextM, nextIndices);
}

static timer t3,t4;

// decides on sparse or dense base on number of nonzeros in the active vertices
template <class F>
vertices edgeMap(wghGraph<intT> GA, vertices V, F add, intT threshold = -1, bool denseForward=0) {
  intT numVertices = GA.n;
  uintT numEdges = GA.m;
  if(threshold == -1) threshold = numEdges/20; //default threshold
  wghVertex<intT> *G = GA.V;
  intT m = V.numNonzeros();
  if (numVertices != V.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }

  // used to generate nonzero indices to get degrees
  uintT* degrees = newA(uintT, m);
  wghVertex<intT>* frontierVertices;
  V.toSparse();
  frontierVertices = newA(wghVertex<intT>,m);
  parallel_for (intT i=0; i < m; i++){
    wghVertex<intT> v = G[V.s[i]];
    degrees[i] = v.degree;
    frontierVertices[i] = v;
  }

  uintT outDegrees = sequence::plusReduce(degrees, m);    
  if (outDegrees == 0) return vertices(numVertices);
  if (m + outDegrees > threshold) {
    t4.start();
    V.toDense();
    free(degrees);
    free(frontierVertices);
    bool* R = denseForward ? 
      edgeMapDenseForward(GA,V.d,add) : 
      edgeMapDense(GA, V.d, add);
    vertices v1 = vertices(numVertices, R);
    //cout << "size (D) = " << v1.m << endl;
    t4.stop();
    return  v1;
  } else {
    t3.start();
    pair<uintT,intT*> R = edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), add);
    //cout << "size (S) = " << R.first << endl;
    free(degrees);
    free(frontierVertices);
    t3.stop();
    return vertices(numVertices, R.first, R.second);
  }
}

template <class F>
void vertexMap(vertices V, F add) {
  intT n = V.numRows();
  intT m = V.numNonzeros();
  if(V.isDense) {
    parallel_for(intT i=0;i<n;i++)
      if(V.d[i]) add(i);
  } else {
    parallel_for(intT i=0;i<m;i++)
      add(V.s[i]);
  }
}

template <class F>
vertices vertexFilter(vertices V, F filter) {
  intT n = V.numRows();
  intT m = V.numNonzeros();
  V.toDense();

  bool* d_out = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) d_out[i] = 0;
  parallel_for(intT i=0;i<n;i++)
    if(V.d[i]) d_out[i] = filter(i);
  
  return vertices(n,d_out);
}
