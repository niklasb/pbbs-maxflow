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
#include "LCP.h"
#include "sequence.h"
#include "blockRadixSort.h"
using namespace std;
using namespace benchIO;

//#define STATS 1

void timeLCP(unsigned char* s, long n, uintT* SA, int rounds, char* outFile) {
  uintT* R;
  //R = LCP(s, SA, n);
  for (int i=0; i < rounds; i++) {
    startTime();
    R = LCP(s, SA, n);
    nextTimeN();
  }

#ifdef STATS
  uintT k = sequence::reduce(R,n,utils::maxF<uintT>());
  long* A = newA(long,n);
  parallel_for(long i=0;i<n;i++) A[i] = R[i];
  long sum = sequence::plusReduce(A,n);
  cout << "max LCP = " << k << " avg LCP = " << (double) sum / (double) (n-1) << endl;
  free(A);

  intSort::iSort(R,n,k+1,utils::identityF<uintT>());

  uintT* offsets = newA(uintT,k+2);
  offsets[k+1] = n;
  parallel_for(long i=0;i<k+1;i++) offsets[i] = UINT_MAX;
  offsets[R[0]] = 0;

  parallel_for(long i=1;i<n;i++) {
    if(R[i] != R[i-1]) offsets[R[i]] = i;
  }

  sequence::scanIBack(offsets, offsets, k+2, 
		     utils::minF<uintT>(), (uintT)UINT_MAX);

  uintT* buckets = newA(uintT,k+1);
  
  parallel_for(long i=0;i<k+1;i++) buckets[i] = offsets[i+1] - offsets[i];

  free(offsets);

  stringstream toWrite;
  for(intT i=0;i<k+1;i++) toWrite<<i<<" "<<buckets[i]<<endl;

  free(buckets);
  ofstream oFile(outFile);
  oFile<<toWrite.str();
  oFile.close();
 
#else


  if (outFile != NULL) writeIntArrayToFile((intT*) R, (intT) n, outFile);
#endif
  cout << endl;
  delete R;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(1);
  char* iFile2 = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  _seq<char> S = readStringFromFile(iFile);
  _seq<uintT> SA = readIntArrayFromFile<uintT>(iFile2);
  if(S.n != SA.n) { 
    cout<<"Lengths don't match\n"; 
    abort(); 
  }
  uintT n = S.n;
  unsigned char* s = newA(unsigned char,n+1);
  parallel_for(uintT i=0;i<n;i++) s[i] = S.A[i];
  s[n] = 0;

  S.del();
  timeLCP(s, n, SA.A, rounds, oFile);
  free(s);
  SA.del();
}
