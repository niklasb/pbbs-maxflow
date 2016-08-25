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
#include "sequence.h"
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "localAlignment.h"
using namespace std;

intT localAlignment(unsigned char* A, long n, unsigned char* B, long m) { 
  n++; m++;
  intT** M = new intT*[n];
  parallel_for(intT i=0;i<n;i++) M[i] = new intT[m];
  parallel_for(intT i=0;i<n;i++) M[i][0] = 0;
  parallel_for(intT j=1;j<m;j++) M[0][j] = 0;
  for(intT k=2;k<n+m-1;k++){
    intT start = (k<=m) ? 1 : k-m+1;
    intT end = (k < n) ? k : n;
    if(end-start > 1000){
      parallel_for(intT i=start;i<end;i++){
	intT j=k-i;
	M[i][j] = max4(0,M[i-1][j-1]+f(A[i-1],B[j-1]),
		       M[i-1][j]+SPACE,M[i][j-1]+SPACE);
      }
    } else {
      for(intT i=start;i<end;i++){
	intT j=k-i;
	M[i][j] = max4(0,M[i-1][j-1]+f(A[i-1],B[j-1]),
		       M[i-1][j]+SPACE,M[i][j-1]+SPACE);
      }
    }
  }

  //compute value of best local alignment 
  intT* rowMaxes = new intT[n];
  parallel_for(intT i=0;i<n;i++) {
    rowMaxes[i] = sequence::reduce(M[i],m,utils::maxF<intT>());
  }
  intT overallMax = sequence::reduce(rowMaxes,n,utils::maxF<intT>());

  delete [] rowMaxes;
  parallel_for(intT i=0;i<n;i++) delete [] M[i];
  delete [] M;

  cout<<overallMax<<endl;
  return overallMax;
}
