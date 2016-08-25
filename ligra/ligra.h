#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <parallel/algorithm>
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "IO.h"
using namespace std;

//*****START FRAMEWORK*****

//*****VERTEX OBJECT*****
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

//*****EDGE FUNCTIONS*****

template <class F, class vertex>
bool* edgeMapDense(graph<vertex> GA, bool* vertices, F f) {
  intT numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  parallel_for (intT i=0; i<numVertices; i++){
    next[i] = 0;
    if (f.cond(i)) { 
      for(intT j=0; j<G[i].getInDegree(); j++){
	intT ngh = G[i].getInNeighbor(j);
	if (vertices[ngh] && f.update(ngh,i)) next[i] = 1;
	if (!f.cond(i)) break;
      }
    }
  }
  return next;
}

template <class F, class vertex>
bool* edgeMapDenseForward(graph<vertex> GA, bool* vertices, F f) {
  intT numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  {parallel_for(long i=0;i<numVertices;i++) next[i] = 0;}
  {parallel_for (long i=0; i<numVertices; i++){
    if (vertices[i]) {
      intT d = G[i].getOutDegree();
      if(d < 10000) {
	for(intT j=0; j<d; j++){
	  uintT ngh = G[i].getOutNeighbor(j);
	  if (f.cond(ngh) && f.updateAtomic(i,ngh)) next[ngh] = 1;
	}
      }
      else {
	parallel_for(intT j=0; j<d; j++){
	  uintT ngh = G[i].getOutNeighbor(j);
	  if (f.cond(ngh) && f.updateAtomic(i,ngh)) next[ngh] = 1;
	}
      }
    }
    }}
  return next;
}

template <class F, class vertex>
pair<uintT,intT*> edgeMapSparse(vertex* frontierVertices, intT* indices, 
			       uintT* degrees, uintT m, F f) {

  uintT* offsets = degrees;
  uintT outEdgeCount = sequence::plusScan(offsets, degrees, m);

  intT* outEdges = newA(intT,outEdgeCount);
  parallel_for (intT i = 0; i < m; i++) {
    intT v = indices[i];
    intT o = offsets[i];
    vertex vert = frontierVertices[i]; 
    intT d = vert.getOutDegree();
    if(d < 10000) {
      for (intT j=0; j < d; j++) {
	intT ngh = vert.getOutNeighbor(j);
	if (f.cond(ngh) && f.updateAtomic(v,ngh)) 
	  outEdges[o+j] = ngh;
	else outEdges[o+j] = -1;
      } 
    } else {
      parallel_for (intT j=0; j < d; j++) {
	intT ngh = vert.getOutNeighbor(j);
	if (f.cond(ngh) && f.updateAtomic(v,ngh)) 
	  outEdges[o+j] = ngh;
	else outEdges[o+j] = -1;
      } 
    }
  }

  intT* nextIndices = newA(intT, outEdgeCount);

  // Filter out the empty slots (marked with -1)
  uintT nextM = sequence::filter(outEdges,nextIndices,outEdgeCount,nonNegF());
  free(outEdges);
  return pair<uintT,intT*>(nextM, nextIndices);
}

static timer t3,t4;
static int edgesTraversed = 0;

// decides on sparse or dense base on number of nonzeros in the active vertices
template <class F, class vertex>
vertices edgeMap(graph<vertex> GA, vertices V, F f, intT threshold = -1, 
		 bool denseForward=0) {
  intT numVertices = GA.n;
  uintT numEdges = GA.m;
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = GA.V;
  intT m = V.numNonzeros();
  if (numVertices != V.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }

  // used to generate nonzero indices to get degrees
  uintT* degrees = newA(uintT, m);
  vertex* frontierVertices;
  V.toSparse();
  frontierVertices = newA(vertex,m);
  parallel_for (long i=0; i < m; i++){
    vertex v = G[V.s[i]];
    degrees[i] = v.getOutDegree();
    frontierVertices[i] = v;
  }
  
  uintT outDegrees = sequence::plusReduce(degrees, m);
  edgesTraversed += outDegrees;
  if (outDegrees == 0) return vertices(numVertices);
  if (m + outDegrees > threshold) { 
    t4.start();
    V.toDense();
    free(degrees);
    free(frontierVertices);
    bool* R = denseForward ? 
      edgeMapDenseForward(GA,V.d,f) : 
      edgeMapDense(GA, V.d, f);
    vertices v1 = vertices(numVertices, R);
    //cout << "size (D) = " << v1.m << endl;
    t4.stop();
    return  v1;
  } else { 
    t3.start();
    pair<uintT,intT*> R = 
      edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), f);
    //cout << "size (S) = " << R.first << endl;
    free(degrees);
    free(frontierVertices);
    t3.stop();
    return vertices(numVertices, R.first, R.second);
  }
}

//*****VERTEX FUNCTIONS*****

template <class F>
void vertexMap(vertices V, F add) {
  intT n = V.numRows();
  uintT m = V.numNonzeros();
  if(V.isDense) {
    parallel_for(long i=0;i<n;i++)
      if(V.d[i]) add(i);
  } else {
    parallel_for(long i=0;i<m;i++)
      add(V.s[i]);
  }
}

template <class F>
vertices vertexFilter(vertices V, F filter) {
  intT n = V.numRows();
  uintT m = V.numNonzeros();
  V.toDense();

  bool* d_out = newA(bool,n);
  parallel_for(long i=0;i<n;i++) d_out[i] = 0;
  parallel_for(long i=0;i<n;i++)
    if(V.d[i]) d_out[i] = filter(i);
  
  return vertices(n,d_out);
}

//*****WEIGHTED EDGE FUNCTIONS*****

template <class F, class vertex>
bool* edgeMapDense(wghGraph<vertex> GA, bool* vertices, F f) {
  intT numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  parallel_for (long i=0; i<numVertices; i++){
    next[i] = 0;
    if (f.cond(i)) {
      for(intT j=0; j<G[i].getInDegree(); j++){
	intT ngh = G[i].getInNeighbor(j);
	if(vertices[ngh]){
	  intT edgeLen = G[i].getInWeight(j);
	  if (vertices[ngh] && f.update(ngh,i,edgeLen)) next[i] = 1;
	}
	if (!f.cond(i)) break;
      }
    }
  }
  return next;
}

template <class F, class vertex>
bool* edgeMapDenseForward(wghGraph<vertex> GA, bool* vertices, F f) {
  intT numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  {parallel_for(long i=0;i<numVertices;i++) next[i] = 0;}
  {parallel_for (long i=0; i<numVertices; i++){
    if (vertices[i]) {
      for(intT j=0; j<G[i].getOutDegree(); j++){
	intT ngh = G[i].getOutNeighbor(j);
	intT edgeLen = G[i].getOutWeight(j);
	if (f.cond(ngh) && f.updateAtomic(i,ngh,edgeLen)) next[ngh] = 1;
      }
    }
    }}
  return next;
}

template <class F, class vertex>
pair<uintT,intT*> edgeMapSparseW(vertex* frontierVertices, intT* indices, uintT* degrees, uintT m, F f) {

  uintT* offsets = degrees;
  uintT outEdgeCount = sequence::plusScan(offsets, degrees, m);

  intT* outEdges = newA(intT,outEdgeCount);

  parallel_for (long i = 0; i < m; i++) {
    intT v = indices[i];
    uintT o = offsets[i];
    vertex vert = frontierVertices[i];
    for (intT j=0; j < vert.getOutDegree(); j++) {
      intT ngh = vert.getOutNeighbor(j);
      intT edgeLen = vert.getOutWeight(j);
      if (f.cond(ngh) && f.updateAtomic(v,ngh,edgeLen)) 
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

// decides on sparse or dense base on number of nonzeros in the active vertices
template <class F, class vertex>
vertices edgeMap(wghGraph<vertex> GA, vertices V, F f, intT threshold = -1, bool denseForward=0) {
  intT numVertices = GA.n;
  uintT numEdges = GA.m;
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = GA.V;
  intT m = V.numNonzeros();
  if (numVertices != V.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }

  // used to generate nonzero indices to get degrees
  uintT* degrees = newA(uintT, m);
  vertex* frontierVertices;
  V.toSparse();
  frontierVertices = newA(vertex,m);

  parallel_for (long i=0; i < m; i++){
    vertex v = G[V.s[i]];
    degrees[i] = v.getOutDegree();
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
      edgeMapDenseForward(GA,V.d,f) : 
      edgeMapDense(GA, V.d, f);
    vertices v1 = vertices(numVertices, R);
    //cout << "size (D) = " << v1.m << endl;
    t4.stop();
    return  v1;
  } else { 
    t3.start();
    pair<uintT,intT*> R = edgeMapSparseW(frontierVertices, V.s, degrees, V.numNonzeros(), f);
    //cout << "size (S) = " << R.first << endl;
    free(degrees);
    free(frontierVertices);
    t3.stop();
    return vertices(numVertices, R.first, R.second);
  }
}
