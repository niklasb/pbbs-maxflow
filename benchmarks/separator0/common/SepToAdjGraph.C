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
#include <cstring>
#include "parallel.h"
#include "IO.h"
#include "graph.h"
#include "graphIO.h"
#include "parseCommandLine.h"
using namespace std;
using namespace benchIO;


void emitAdjGraph(char* iFile, char* sFile, char* oFile) {
  graph<intT> G = readGraphFromFile<intT>(iFile);
  _seq<intT> sepMap = readIntArrayFromFile<intT>(sFile);

  intT* oldE = G.edges();
  intT n = G.n;
  vertex<intT> *v = newA(vertex<intT>,G.n);
  parallel_for(uintT i=0; i < n; i++) {
    intT oldID = sepMap.A[i];
    vertex<intT> oldV = G.V[oldID];
    intT* oldNgh = oldV.Neighbors;
    intT* newNgh = newA(intT, oldV.degree);
    for (intT j=0; j < oldV.degree; j++) {
      newNgh[j] = sepMap.A[oldNgh[j]];
    }
    v[i].Neighbors = newNgh;
    v[i].degree = oldV.degree;
  }
  graph<intT> newG = graph<intT>(v, G.n, G.m);
  writeGraphToFile(newG, oFile);
  G.del();
  newG.del();
  cout << "here" << endl;  
}


int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o] <inFile> <sepfile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* sFile = fnames.second;
  char* oFile = P.getOptionValue("-o");
  cout << "sFile: " << sFile << endl;
  emitAdjGraph(iFile,sFile,oFile);
  return 1;
}
