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

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "bytecode.h"
#include "quickSort.h"
#include "utils.h"
#include "graph.h"
#include "graphTesting.h"
using namespace std;

template<class F>
struct touchT {
  bool srcTarg(F f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
    return (src > target);
  }
  bool srcTarg(F f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
    return (src > target);
  }
};

template <class vertex>
void touchGraph(graph<vertex> G) {
  vertex *V = G.V;
  parallel_for (long i = 0; i < G.n; i++) {
    char *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
#ifdef WEIGHTED
    decodeWgh(touchT<printF>(), printF(), nghArr, i, d);
#else
    decode(touchT<printF>(), printF(), nghArr, i, d);
#endif
  }
  parallel_for (long i = 0; i < G.n; i++) {
    char *nghArr = V[i].getInNeighbors();
    uintT d = V[i].getInDegree();
#ifdef WEIGHTED
    decodeWgh(touchT<printF>(), printF(), nghArr, i, d);
#else
    decode(touchT<printF>(), printF(), nghArr, i, d);
#endif
  }
}

void touchMemory(long size) {
  char* A = (char*) malloc(size);
  if(A == NULL) return;
  parallel_for(long i=0;i<size;i++) A[i] = 1;
  free(A);
}

template <class vertex>
graph<vertex> readGraph(char* fname, bool isSymmetric) {
  ifstream in(fname,ifstream::in |ios::binary);
  in.seekg(0,ios::end);
  long size = in.tellg();
  in.seekg(0);
  cout << "size = " << size << endl;
  char* s = (char*) malloc(size);
  in.read(s,size);
  long* sizes = (long*) s;
  long n = sizes[0], m = sizes[1], totalSpace = sizes[2];

  cout << "n = "<<n<<" m = "<<m<<" totalSpace = "<<totalSpace<<endl;
  cout << "reading file..."<<endl;

  uintT* offsets = (uintT*) (s+3*sizeof(long));
  long skip = 3*sizeof(long) + (n+1)*sizeof(intT);
  uintE* Degrees = (uintE*) (s+skip);
  skip+= n*sizeof(intE);
  char* edges = s+skip;

  uintT* inOffsets;
  char* inEdges;
  uintE* inDegrees;
  if(!isSymmetric){
    skip += totalSpace;
    char* inData = s + skip;
    sizes = (long*) inData;
    long inTotalSpace = sizes[0];
    cout << "inTotalSpace = "<<inTotalSpace<<endl;
    skip += sizeof(long);
    inOffsets = (uintT*) (s + skip);
    skip += (n+1)*sizeof(uintT);
    inDegrees = (uintE*)(s+skip);
    skip += n*sizeof(uintE);
    inEdges = s + skip;
  } else {
    inOffsets = offsets;
    inEdges = edges;
    inDegrees = Degrees;
  }

  in.close();

  cout << "creating graph..."<<endl;
  graph<vertex> G(inOffsets,offsets,inEdges,edges,n,m,inDegrees,Degrees,s);

  //becomes a lot slower than original code if this is not included
  parallel_for(int i=0;i<50000;i++) touchGraph(G); 
  touchMemory(size*2); //weird memory effect
  
  return G;
}
