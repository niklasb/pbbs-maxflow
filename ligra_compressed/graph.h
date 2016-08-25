#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "bytecode.h"
#include "parallel.h"

using namespace std;

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

struct symmetricVertex {
  char* neighbors;
  uintT degree;
  char* getInNeighbors() { return neighbors; }
  char* getOutNeighbors() { return neighbors; }
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInNeighbors(char* _i) { neighbors = _i; }
  void setOutNeighbors(char* _i) { neighbors = _i; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
};

struct asymmetricVertex {
  char* inNeighbors;
  char* outNeighbors;
  uintT outDegree;
  uintT inDegree;
  char* getInNeighbors() { return inNeighbors; }
  char* getOutNeighbors() { return outNeighbors; }
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInNeighbors(char* _i) { inNeighbors = _i; }
  void setOutNeighbors(char* _i) { outNeighbors = _i; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); 
    swap(inDegree,outDegree); }
};

template <class vertex>
struct graph {
  vertex *V;
  long n;
  long m;
  uintT* inOffsets, *outOffsets;
  char* inEdges, *outEdges;
  uintE* flags;
  char* s;
graph(uintT* _inOffsets, uintT* _outOffsets, char* _inEdges, char* _outEdges, long nn, long mm, uintE* inDegrees, uintE* outDegrees, char* _s) 
: inOffsets(_inOffsets), outOffsets(_outOffsets), inEdges(_inEdges), outEdges(_outEdges), n(nn), m(mm), s(_s), flags(NULL) {
  V = newA(vertex,n);
  parallel_for(long i=0;i<n;i++) {
    long o = outOffsets[i];
    uintT d = outDegrees[i];
    V[i].setOutDegree(d);
    V[i].setOutNeighbors(outEdges+o);
  }

  if(sizeof(vertex) == sizeof(asymmetricVertex)){
    parallel_for(long i=0;i<n;i++) {
      long o = inOffsets[i];
      uintT d = inDegrees[i];
      V[i].setInDegree(d);
      V[i].setInNeighbors(inEdges+o);
    }
  }
}

  void del() {
    free(s);
    free(V);
  }
  void transpose() {
    if(sizeof(vertex) == sizeof(asymmetricVertex))
      parallel_for(long i=0;i<n;i++)
	V[i].flipEdges();
  }
};
#endif
