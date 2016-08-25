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
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include "IO.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "Separator.h"
using namespace std;
using namespace benchIO;

// Make sure the ordering is proper
void checkOrder(uintT* I, uintT n) {
  uintT* II = newA(uintT,n);
  for (uintT i=0; i < n; i++) II[i] = I[i];
  sort(II,II+n);
  for (uintT i=0; i < n; i++)
    if (II[i] != i) {
      cout << "Bad ordering at i = " << i << " val = " << II[i] << endl;
      break;
    }
  free(II);
}

void timeSeparator(edgeArray<uintT> G, int rounds, char* outFile) {
  uintT* I = separator(G);
  //rounds = 0;
  for (int i=0; i < rounds; i++) {
    free(I);
    startTime();
    I = separator(G);
    nextTimeN();
  }
  cout << endl;
  intT n = max(G.numRows,G.numCols);
  G.del();
  //checkOrder(I,n);
  if(outFile != NULL) writeIntArrayToFile<intT>((intT*)I,n,outFile);
  free(I);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, "[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  edgeArray<uintT> G = readEdgeArrayFromFile<uintT>(iFile);
  timeSeparator(G, rounds, oFile);
}




