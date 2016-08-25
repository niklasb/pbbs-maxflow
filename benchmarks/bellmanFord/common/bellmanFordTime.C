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
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include "IO.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "bellmanFord.h"
using namespace std;
using namespace benchIO;

void timeBellmanFord(wghGraph<intT> WG, int rounds, char* outFile) {
  intT* ShortestPathLen;
  for (int i=0; i < rounds; i++) {
    startTime();
    ShortestPathLen = bellmanFord(1, WG);
    free(ShortestPathLen);
    nextTimeN();
  }
  cout << endl;
  WG.del();
  //if(outFile != NULL) writeIntArrayToFile(ShortestPathLen, WG.n, outFile);
}


wghGraph<intT> readGraphFromBinary(char* iFile) {
  char* config = ".config";
  char* adj = ".adj";
  char* idx = ".idx";
  char configFile[strlen(iFile)+7];
  char adjFile[strlen(iFile)+4];
  char idxFile[strlen(iFile)+4];
  strcpy(configFile,iFile);
  strcpy(adjFile,iFile);
  strcpy(idxFile,iFile);
  strcat(configFile,config);
  strcat(adjFile,adj);
  strcat(idxFile,idx);

  ifstream in(configFile, ifstream::in);
  intT n;
  in >> n;
  in.close();
  cout << n<<endl;
  ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
  uintT m = size/sizeof(uint);
  char* s = (char *) malloc(size);  
  in2.read(s,size);
  in2.close();
  
  intT* edges = (intT*) s;

  ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  utils::myAssert(n == size/sizeof(long), "File size wrong");

  char* t = (char *) malloc(size);  
  in3.read(t,size);
  in3.close();
  long* offsets = (long*) t;


  wghVertex<intT> *V = newA(wghVertex<intT>, n);
  parallel_for(intT i=0;i<n;i++) {
    long o = offsets[i];
    long l = ((i==n-1) ? m : offsets[i+1])-offsets[i];
    V[i].degree = l;
    V[i].Neighbors = edges+o;  

  }

  free(offsets);
  cout << "n = "<<n<<" m = "<<m<<endl;
  int* weights = newA(int,m);

  parallel_for(intT i=0;i<m;i++) weights[i] = utils::hash(i)%utils::log2Up(n);
  parallel_for(intT i=0;i<n;i++) {
    long o = V[i].Neighbors - edges;
    V[i].nghWeights = weights + o;
  }

  wghGraph<intT> G = wghGraph<intT>(V,(intT)n,(intT)m,edges,weights);

  // checkSymmetric(G);
  // graphCheckConsistency(G);  
  return G;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  int binary = P.getOption("-b");
  wghGraph<intT> WG(0,0,0);
  if(binary) WG = readGraphFromBinary(iFile);
  else WG = readWghGraphFromFile<intT>(iFile);

  timeBellmanFord(WG, rounds, oFile);
}
