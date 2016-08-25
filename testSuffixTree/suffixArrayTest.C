// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch, Julian Shun and the PBBS team
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
#include <cstdlib>
#include "gettime.h"
#include "sequence.h"
#include "stringGen.h"
#include <string.h>
using namespace std;

uintT* suffixArray(uintT* ss, long n);

int cilk_main(int argc, char **argv) {
  if (argc < 2){
    cout<<"Usage: ./suffixArrayTest <filename>\n";
  }
  else {
    char* filename = (char*)argv[1];
    _seq<char> str = dataGen::readCharFile(filename);
    uintT n = str.n;
    uintT *ss = newA(uintT, n+3); 
    ss[n] = ss[n+1] = ss[n+2] = 0;
    parallel_for (long i=0; i < n; i++) ss[i] = ((uintT) str.A[i])+1;
    str.del();
    for(int r=0;r<4;r++){
      startTime();
      uintT* SA = suffixArray(ss, n);
      stopTime(1,"Suffix Array (from file)");
      free(SA); 
    }
    free(ss);
    return 0;
  }


}
