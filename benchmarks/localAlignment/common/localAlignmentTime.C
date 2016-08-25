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
#include "parallel.h"
#include "IO.h"
#include "parseCommandLine.h"
#include "localAlignment.h"
#include "sequence.h"
using namespace std;
using namespace benchIO;

void timeLocalAlignment(unsigned char* s1, long n, unsigned char* s2, long m, int rounds, char* outFile) {
  intT R[0];
  R[0] = localAlignment(s1, n, s2, m);
  for (int i=0; i < rounds; i++) {
    startTime();
    R[0] = localAlignment(s1, n, s2, m);
    nextTimeN();
  }
  cout << endl;
  if (outFile != NULL) writeIntArrayToFile((intT*) R, (intT) 1, outFile);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-p] [-o <outFile>] [-r <rounds>] <inFile1> <inFile2>");
  char* iFile1 = P.getArgument(1);
  char* iFile2 = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  const _seq<char> S1 = readStringFromFile(iFile1);
  const _seq<char> S2 = readStringFromFile(iFile2);  

  if(S1.n > S2.n) {
    timeLocalAlignment((unsigned char*) S2.A, S2.n, (unsigned char*) S1.A, S1.n, rounds, oFile);
  }
  else timeLocalAlignment((unsigned char*) S1.A, S1.n, (unsigned char*) S2.A, S2.n, rounds, oFile);

}
