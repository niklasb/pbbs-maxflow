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
#include "parallel.h"
#include "parseCommandLine.h"
#include "loadLassoData.h"

using namespace std;

// Checker program for lasso-regression.
// Essentially checks if the objective value
// is reasonable (has to be provided separately)

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv);
  pair<char*,char*> fnames = P.IOFileNames();
  double minimumAcceptableObjective = P.getOptionDoubleValue("-m", -1);
  double lambda = P.getOptionDoubleValue("-l", -1);
  if (minimumAcceptableObjective < 0) {
    cout << "lassoCheck: you must provide minimum acceptable objective with switch -m" << endl;
    abort();
  }
  if (lambda < 0) {
    cout << "lassoCheck: you need to provide lambda as switch -l" << endl;
    abort();
  }
  int rounds = P.getOptionIntValue("-r",1);
  
  // In-file contains the matrix (fnames.first)
  // Out-file contains the computed weights (fnames.second)
  
  std::vector<sparse_array> cols, rows;
  std::vector<valuetype_t> y;
  int nx, ny;
  load_lasso_data(fnames.first, &cols, &rows, &y, nx, ny);
  
  std::vector<valuetype_t> weights = std::vector<valuetype_t>(nx, 0.0);
  
  // Load output, i.e weights computed by the algorithm
  FILE * f = fopen(fnames.second, "r");
  if (f == NULL) {
      cout << "File " << std::string(fnames.second) << " not found! " << std::endl;
      exit(1);
  }
  
  // Maybe not the cleanest data loading... 
  char s[255];
  char delims[] = ",";	
  char *t = NULL;
  double l1 = 0.0;
  while(fgets(s, 255, f) != NULL) {
     FIXLINE(s);
     t = strtok(s,delims);
     int i;
     sscanf(t, "%d", &i); 
     t = strtok(NULL, delims);
     double val;
     sscanf(t, "%lf", &val);
     assert(i<nx);
     weights[i] = val;
     // Update L1 penalty
     l1 += std::abs(val);
  }
  fclose(f);
  
  // Compute least squares
  double l2err = 0.0;
  for(int i=0; i<ny; i++) {
    sparse_array &row = rows[i];
    double s = 0.0;
    for(int j=0;j<row.length();j++) {
       s += weights[row.idxs[j]] * row.values[j];
    }
    s -= y[i];
   
    s *= s;
    l2err += s;
  }
  
  double objective = l2err + l1*lambda;
  
  if (objective>minimumAcceptableObjective) {
       cout << "lassoCheck: FAILED with objective value: " << objective << " l1:" << l1 << " (min acceptable: " << 
                minimumAcceptableObjective << ")" << endl;
       abort();
  } else {
    // Passed. But do not output anything, otherwise benchmark thinks it failed.
  }
}
