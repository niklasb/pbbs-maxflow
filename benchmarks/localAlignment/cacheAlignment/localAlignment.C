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

static intT nm;
static intT n_choose_2;
static intT n_plus_m;

inline intT getIndex(const intT n, const intT m, const intT i, const intT k){
  // intT maxLen = min(n,m);
  // intT larger = max(n,m);
  intT offset;
  if (k <= n) offset = k*(k+1)/2;
  else if(k > m) offset = nm - (n_plus_m-k)*(n_plus_m-k-1)/2;
  else offset = n_choose_2 + (k+1-n)*n;
  return offset + i;
}

intT localAlignment(unsigned char* A, long n, unsigned char* B, long m) { 
  n++; m++;
  nm = n*m; n_choose_2 = n*(n-1)/2; n_plus_m = n+m;
  intT* M = new intT[n*m];
  parallel_for(intT i=0;i<n;i++) {
    M[getIndex(n,m,i,i)] = 0;
  }
  parallel_for(intT j=1;j<m;j++) {
    M[getIndex(n,m,0,j)] = 0;
  }

  // intT larger = max(n,m);
  // intT smaller = min(n,m);

  for(intT k=2;k<n_plus_m-1;k++){
    intT start = (k<m) ? 1 : 0;
    intT end;// = (k<n) ? k : n;
    if(k < n) end = k;
    else if (k < m) end = n;// smaller;
    else end = n_plus_m-k-1;

    if(k < m) {
      if(end-start > 10000) {
	parallel_for(intT i=start;i<end;i++){
	  intT j = min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i-1,k-2)]+f(A[i-1],B[j-1]),
				      M[getIndex(n,m,i-1,k-1)]+SPACE,
				      M[getIndex(n,m,i,k-1)]+SPACE);
	}
      } else {
	for(intT i=start;i<end;i++){
	  intT j = min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i-1,k-2)]+f(A[i-1],B[j-1]),
				      M[getIndex(n,m,i-1,k-1)]+SPACE,
				      M[getIndex(n,m,i,k-1)]+SPACE);
	}
      }
    }
    else if (k==m) {
      if(end-start > 10000) {
	parallel_for(intT i=start;i<end;i++){
	  // intT ii = k-m+i; intT jj = m-1-i;
	  intT j = m-1-i;//min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i,k-2)]+f(A[i],B[j]),
				      M[getIndex(n,m,i,k-1)]+SPACE,
				      M[getIndex(n,m,i+1,k-1)]+SPACE);

	}
      } else {
	for(intT i=start;i<end;i++){
	  // intT ii = k-m+i; intT jj = m-1-i;
	  intT j = m-1-i;//min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i,k-2)]+f(A[i],B[j]),
				      M[getIndex(n,m,i,k-1)]+SPACE,
				      M[getIndex(n,m,i+1,k-1)]+SPACE);
	}
      }
    }
      
    else {
      if(end-start > 10000) {
	parallel_for(intT i=start;i<end;i++){
	  intT ii = k-m+i; intT jj = k-ii-2;
	  //intT j = min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i+1,k-2)]+f(A[ii],B[jj]),
				      M[getIndex(n,m,i,k-1)]+SPACE,
				      M[getIndex(n,m,i+1,k-1)]+SPACE);
	}
      } else {
	for(intT i=start;i<end;i++){
	  intT ii = k-m+i; intT jj = k-ii-2;
	  //intT j = min((intT)m-1,k-i);
	  M[getIndex(n,m,i,k)] = max4(0,M[getIndex(n,m,i+1,k-2)]+f(A[ii],B[jj]),
				      M[getIndex(n,m,i,k-1)]+SPACE,
				      M[getIndex(n,m,i+1,k-1)]+SPACE);
	}
      }
    } 
  }


  // for(intT k=0;k<n+m-1;k++){
  //   intT start = 0;
  //   intT end;// = (k<n) ? k : n;
  //   if(k < n) end = k+1;
  //   else if (k < m) end = n;// smaller;
  //   else end = n+m-k-1;
  //   cout<<"k="<<k<<": ";
  //   for(intT i=start;i<end;i++){
  //     intT j = min((intT)m-1,k-i);
  //     cout << M[getIndex(n,m,i,k)]<< " ";
  //   }
  //   cout<<endl;
  // }

  //compute value of best local alignment 
  intT overallMax = sequence::reduce(M,n*m,utils::maxF<intT>());

  delete [] M;

  cout<<overallMax<<endl;
  return overallMax;
}
