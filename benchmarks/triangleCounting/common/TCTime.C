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
#include <math.h>
#include "binaryReader.h"
using namespace std;
using namespace benchIO;

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

double error(long approxCounts[], long trueCount, int rounds) {
  long total = 0;
  double error = 0.0;
  for(int i=0;i<rounds;i++) error += fabs(((double)approxCounts[i]-(double)trueCount)/(double)trueCount);
  return error/rounds;
}

double errorMean(long approxCounts[], long trueCount, int rounds) {
  long total = 0;
  double error = 0.0;
  for(int i=0;i<rounds;i++) error += ((double)approxCounts[i]-(double)trueCount)/(double)trueCount;
  return error/rounds;
}

double errorVariance(long approxCounts[], long trueCount, int rounds) {
  double mean = errorMean(approxCounts,trueCount,rounds);
  double sum = 0;

  for(int i=0;i<rounds;i++) {
    sum += pow((((double)approxCounts[i] - (double)trueCount)/(double)trueCount) - mean,2);
  }
  return sum/rounds;
}

void timeTC(graphC<uintT,uint> G, int rounds, char* outFile, int denom) {
  //graphC<uintT> GN = G.copy();
  long trueCount;
  trueCount = countTriangle(G,(double)1.0,(long)0);
  long approxCounts[rounds];
  for (int i=0; i < rounds; i++) {
    startTime();
    approxCounts[i] = countTriangle(G,(double)1/denom,(long)i*G.n);
    nextTimeN();
  }
  G.del();
  //GN.del();

  double e = error(approxCounts,trueCount,rounds);
  cout << "error = " << std::setprecision(6) << 100*e << "%"<<endl;
  double v = errorVariance(approxCounts,trueCount,rounds);
  cout << "variance = " << std::setprecision(6) << 100*v << "%"<<endl;
  cout << endl;
  if (outFile != NULL) writeAnswer(trueCount, outFile);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  int denom = P.getOptionIntValue("-d",25);
  bool binary = P.getOption("-b");
  graphC<uintT,uint> G = 
    (binary ? readGraphCFromBinary(iFile) : 
     readGraphCFromFile<uintT,uint>(iFile));
  timeTC(G, rounds, oFile, denom);
}
