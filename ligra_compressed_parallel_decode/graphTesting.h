#ifndef GRAPHTESTING_H
#define GRAPHTESTING_H

#include "graph.h"
#include "bytecode.h"

template<class F>
struct printT {
  bool srcTarg(F f, intE src, intE target, intT edgeNumber) {
    cout << target <<" ";
    return true;
  }
  bool srcTarg(F f, intE src, intE target, intE weight, intT edgeNumber) {
    cout << target <<" ";
    return true;
  }
};

struct printF {};

template <class vertex>
void printGraph(graph<vertex> G) {
  vertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    char *nghArr = V[i].getOutNeighbors();
    intT d = V[i].getOutDegree();
    cout << i << ": ";
#ifdef WEIGHTED
    decodeWgh(printT<printF>(), printF(), nghArr, i, d);
#else
    decode(printT<printF>(), printF(), nghArr, i, d);
#endif
    cout << endl;
  }
}

template<class F>
struct checkT {
  long n,m;
checkT(long nn, long mm) : n(nn), m(mm) {} 
  bool srcTarg(F f, intE src, intE target, intT edgeNumber) {
    if(target < 0 || target >= n) 
      {cout << "out of bounds: ("<< src << "," << 
	  target << ") "<< endl; abort(); } 
    return 1;
  }
  bool srcTarg(F f, intE src, intE target, intE weight, intT edgeNumber) {
    if(target < 0 || target >= n) 
      {cout << "out of bounds: ("<< src << ","<< 
	  target << ","<<weight << ") "<<endl; abort(); } 
    return 1;
  }
};

template <class vertex>
void checkGraph(graph<vertex> G, long n, long m) {
  vertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    char *nghArr = V[i].getOutNeighbors();
    intT d = V[i].getOutDegree();
#ifdef WEIGHTED
    decodeWgh(checkT<printF>(n,m), printF(), nghArr, i, d);
#else
    decode(checkT<printF>(n,m), printF(), nghArr, i, d);
#endif
  }
  for (long i = 0; i < G.n; i++) {
    char *nghArr = V[i].getInNeighbors();
    intT d = V[i].getInDegree();
#ifdef WEIGHTED
    decodeWgh(checkT<printF>(n,m), printF(), nghArr, i, d);
#else
    decode(checkT<printF>(n,m), printF(), nghArr, i, d);
#endif
  }
}

#endif
