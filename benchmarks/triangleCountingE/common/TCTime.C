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
#include "TC.h"
#include "graphUtils.h"

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "serialSort.h"
#endif

using namespace std;
using namespace benchIO;

struct notNeg {
  bool operator() (intT i){
    return i != -1;
  };
};

struct edgeLexico {
  bool operator() (edge<intT> a, edge<intT> b) {
    return (a.u == b.u) ? (a.v < b.v) : a.u < b.u;
  }; 
};

void writeAnswer(intT ans, char *oFile)
{
  ofstream outF(oFile);
  
  if (outF.is_open()) {
    outF << ans << endl;
  }
  else {
    cerr << "ERROR: Cannot open output file" << endl;
    exit(0xff);
  }
}

void timeTC(edgeArray<intT> EA, intT* degrees, int rounds, char* outFile) {
  intT ans;
  edge<intT>* E = newA(edge<intT>,EA.nonZeros);
  parallel_for(intT i=0;i<EA.nonZeros;i++) E[i] = EA.E[i];
  intT* degrees2 = newA(intT,EA.numRows);
  parallel_for(intT i=0;i<EA.numRows;i++) degrees2[i] = degrees[i];
  ans = countTriangle(edgeArray<intT>(E,EA.numRows,EA.numCols,EA.nonZeros), degrees2);

  for (int i=0; i < rounds; i++) {
    parallel_for(intT i=0;i<EA.nonZeros;i++) E[i] = EA.E[i];
    parallel_for(intT i=0;i<EA.numRows;i++) degrees2[i] = degrees[i];

    startTime();
    ans = countTriangle(edgeArray<intT>(E,EA.numRows,EA.numCols,EA.nonZeros), degrees2);
    nextTimeN();
  }

  cout << endl; 
  free(E);
  free(degrees2);
  if (outFile != NULL) writeAnswer(ans, outFile);
}

struct directedEdge {
  intT* degrees;
  directedEdge (intT* _degrees)
    : degrees(_degrees) {};

  bool operator() (edge<intT> e){
    return e.u < e.v;
    //return (degrees[e.u] == degrees[e.v]) ? e.u < e.v : degrees[e.u] < degrees[e.v];
  };
};

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  edgeArray<intT> EA_in = readEdgeArrayFromFile<intT>(iFile);

  edgeArray<intT> EA = makeSymmetric(EA_in);
  EA_in.del();

  edge<intT>* E = EA.E;
  intT m = EA.nonZeros;
  intT n = max(EA.numRows,EA.numCols);

  //compute original degrees
  compSort(E, m, edgeLexico());

  intT* flags = newA(intT,m);
  flags[0] = 0;
  parallel_for(intT k=1;k<m;k++) 
    if(E[k].u != E[k-1].u) flags[k] = k; else flags[k] = -1;

  intT* flags2 = newA(intT,n+1);
  intT uniqueFlags = sequence::filter(flags,flags2,m,notNeg());
  flags2[uniqueFlags] = m;

  intT* degrees = newA(intT,n);
  parallel_for(intT k=0;k<n;k++) degrees[k] = 0;
  parallel_for(intT k=0;k<uniqueFlags;k++)
    degrees[E[flags2[k]].u] = flags2[k+1]-flags2[k];

  //create directed version
  edge<intT>* E_directed = newA(edge<intT>, m);
  m = sequence::filter(E,E_directed,m,directedEdge(degrees));
  free(E);
  EA.E = E_directed; EA.nonZeros = m;

  //compute new degrees
  flags[0] = 0;
  parallel_for(intT k=1;k<m;k++) 
    if(E_directed[k].u != E_directed[k-1].u) flags[k] = k; else flags[k] = -1;

  uniqueFlags = sequence::filter(flags,flags2,m,notNeg());
  flags2[uniqueFlags] = m;

  parallel_for(intT k=0;k<n;k++) degrees[k] = 0;
  parallel_for(intT k=0;k<uniqueFlags;k++)
    degrees[E_directed[flags2[k]].u] = flags2[k+1]-flags2[k];
    
  free(flags); free(flags2);
  timeTC(EA, degrees, rounds, oFile);
  EA.del();
  free(degrees);
}
