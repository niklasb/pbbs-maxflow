
#ifndef A_QSORT_INCLUDED
#define A_QSORT_INCLUDED
#include "utils.h"
#include "cilk.h"

template <class E, class BinPred>
void insertionSort(E* A, int n, BinPred f) {
  for (int i=0; i < n; i++) {
    E v = A[i];
    E* B = A + i;
    while (B-- > A && f(v,*B)) *(B+1) = *B;
    *(B+1) = v;
  }
}

#define ISORT 32

// Quicksort based on median of three elements as pivot
//  and uses insertionSort for small inputs
template <class E, class BinPred>
void quickSort(E* A, int n, BinPred f) {
  if (n < ISORT) insertionSort(A, n, f);
  else {
    E p = std::__median(A[n/4],A[n/2],A[(3*n)/4],f);
    E* L = A;   // below L are less than pivot
    E* M = A;   // between L and M are equal to pivot
    E* R = A+n-1; // above R are greater than pivot
    while (1) {
      while (!f(p,*M)) {
	if (f(*M,p)) swap(*M,*(L++));
	if (M >= R) break; 
	M++;
      }
      while (f(p,*R)) R--;
      if (M >= R) break; 
      swap(*M,*R--); 
      if (f(*M,p)) swap(*M,*(L++));
      M++;
    }
    cilk_spawn quickSort(A, L-A, f);
    quickSort(M, A+n-M, f); // Exclude all elts that equal pivot
    cilk_sync;
  }
}

#define compSort(__A, __n, __f) (quickSort(__A, __n, __f))

#endif // _A_QSORT_INCLUDED
