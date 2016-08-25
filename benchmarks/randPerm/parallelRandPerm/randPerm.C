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


#include "parallel.h"
#include "utils.h"
#include "randPerm.h"
#include <iostream>
#include "sequence.h"
using namespace std;

template <class E>
void randPerm(E *A, intT n) {
  intT *I = newA(intT,n);
  intT *H = newA(intT,n);
  intT *check = newA(intT,n);

  cilk_for (intT i=0; i < n; i++) {
    H[i] = utils::hash(i)%(i+1);
    I[i] = i;
    check[i] = i;
  }

  intT end = n;
  intT ratio = 50;
  intT maxR = 1 + n/ratio;
  intT wasted = 0;
  intT round = 0;
  intT *hold = newA(intT,maxR);
  bool *flags = newA(bool,maxR);
  //intT *H = newA(intT,maxR);

  while (end > 0) {
    round++;
    //if (round > 10 * ratio) abort();
    intT size = 1 + end/ratio;
    intT start = end-size;

    {cilk_for(intT i = 0; i < size; i++) {
      intT idx = I[i+start];
      intT idy = H[idx];
      //intT old = check[idy];
      //utils::CAS(&check[idy],old,idx);
      utils::writeMax(&check[idy], idx);
      }}

    {cilk_for(intT i = 0; i < size; i++) {
      intT idx = I[i+start];
      intT idy = H[idx];
      flags[i] = 1;
      hold[i] = idx;
      if (check[idy] == idx ) {
	if (check[idx] == idx) {
	  swap(A[idx],A[idy]);
	  flags[i] = 0;
	}
	check[idy] = idy;
      }
      }}
    intT nn = sequence::pack(hold,I+start,flags,size);
    end = end - size + nn;
    wasted += nn;
  }
  free(H); free(I); free(check); free(hold); free(flags);
  //cout << "wasted = " << wasted << " rounds = " << round  << endl;
}

template void randPerm<intT>(intT*,intT);
template void randPerm<double>(double*,intT);
