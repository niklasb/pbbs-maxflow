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

#include "gettime.h"
#include "parallel.h"
#include "utils.h"
#include "randPerm.h"
#include <iostream>
#include "sequence.h"
#include "sampleSort.h"
using namespace std;

inline long hashLong(long a)
{
   a = (a+0xa47f80cd7ed55d16) + (a<<24);
   a = (a^0xef7a98cbc761c23c) ^ (a>>19);
   a = (a+0x20dca4a3165667b1) + (a<<37);
   a = (a+0x57ac8d9ad3a2646c) ^ (a<<49);
   a = (a+0x6b4c90adfd7046c5) + (a<<3);
   a = (a^0x1fb1897eb55a4f09) ^ (a>>61);
   return a;
}


template <class E>
struct pairLess {
  bool operator() (pair<E,long> a, pair<E,long> b) {
    return a.second < b.second;
  }
};

template <class E>
void randPerm(E *A, intT n) {
  
  typedef pair<E,long> pairLong;

  pairLong* B = newA(pairLong,n);

  {parallel_for (intT i=0; i < n; i++) {
      B[i] = make_pair(A[i],hashLong((long)i));
    }}

  compSort(B,n,pairLess<E>());

  {parallel_for (intT i=0;i<n;i++) A[i] = B[i].first;}

  free(B);
}

template void randPerm<intT>(intT*,intT);
template void randPerm<double>(double*,intT);
