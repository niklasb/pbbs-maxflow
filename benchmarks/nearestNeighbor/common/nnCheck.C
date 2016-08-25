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
#include "sequence.h"
#include "utils.h"
#include "cilk.h" 
#include "IO.h"
#include "parseCommandLine.h"
#include <cmath>
#include "PointGenerator.h"
 
using namespace std;
using namespace benchIO;


// Checker program for NN is actually a fake-checker. The checking is done
// directly by the timer which outputs "1" or "0" depending on the success.

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv);
  pair<char*,char*> fnames = P.IOFileNames();
   
  FILE * outf = fopen(fnames.second, "r");
  if (outf != NULL) {
      int succ;
      fscanf(outf, "%d\n", &succ);
      if (succ == 0) 
        std::cout << "Failure!" << std::endl;
      
  } else {
    std::cout << "Outfile was not found!" << std::endl;
    return 1;
  }
  return 0;
}
