#include <iostream>
#include "cilk.h"
#include "utils.h"
#include "sequence.h"
using namespace std;

template <class E>
void randPerm(E *A, int n) {
  int *I = newA(int,n);
  int *H = newA(int,n);
  int *check = newA(int,n);
  if (n < 100000) {
    for (int i=n-1; i > 0; i--) 
      swap(A[utils::hash(i)%(i+1)],A[i]);
    return;
  }

  cilk_for (int i=0; i < n; i++) {
    H[i] = utils::hash(i)%(i+1);
    I[i] = i;
    check[i] = i;
  }

  int end = n;
  int ratio = 100;
  int maxR = 1 + n/ratio;
  int wasted = 0;
  int round = 0;
  int *hold = newA(int,maxR);
  bool *flags = newA(bool,maxR);
  //int *H = newA(int,maxR);

  while (end > 0) {
    round++;
    //if (round > 10 * ratio) abort();
    int size = 1 + end/ratio;
    int start = end-size;

    {cilk_for(int i = 0; i < size; i++) {
      int idx = I[i+start];
      int idy = H[idx];
      utils::writeMax(&check[idy], idx);
      }}

    {cilk_for(int i = 0; i < size; i++) {
      int idx = I[i+start];
      int idy = H[idx];
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
    int nn = sequence::pack(hold,I+start,flags,size);
    end = end - size + nn;
    wasted += nn;
  }
  free(H); free(I); free(check); free(hold); free(flags);
  //cout << "wasted = " << wasted << " rounds = " << round  << endl;
}
