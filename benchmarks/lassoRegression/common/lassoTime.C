// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and the PBBS team
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
#include "parseCommandLine.h"
#include "loadLassoData.h"

using namespace std;

void lassoRead(char * inputfile) {
  load_lasso_data(inputfile, &A_cols, &A_rows, &y, nx, ny);
}

void lassoWriteResults(char * outputfile) {
  // Output results
  if (outputfile != NULL) {
    FILE * outf = fopen(outputfile, "w");
    for(int i=0; i<nx; i++) {
      if (x[i] != 0.0) fprintf(outf, "%d,%lf\n", i, x[i]); 
    }
    fclose(outf);
  }
}

void timeLasso(char * inFile, char * outFile, int rounds, double lambda, double accuracy) {
  double dataLoadingTimes = 0;
  lassoRead(inFile);

  for (int i=0; i < rounds; i++) {
    startTime();
    solveLasso(lambda, accuracy);
    nextTimeN();
    if (outFile != NULL)
        lassoWriteResults(outFile);
  }
  cout << endl;    
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-o <outFile>] [-r <rounds>] [-a <optgoal>] [-l <lambda>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  double lambda = P.getOptionDoubleValue("-l", 0.5);
  int rounds = P.getOptionIntValue("-r",1);
  double accuracy = P.getOptionDoubleValue("-a",0.0);
  timeLasso(iFile, oFile, rounds, lambda, accuracy);
}
