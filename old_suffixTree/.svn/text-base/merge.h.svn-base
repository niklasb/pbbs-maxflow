#include "cilk.h"
#define _MERGE_BSIZE 8192

template <class ET, class F> 
int binSearchOld(ET* S, int n, ET v, F f) {
  if (n == 0) return 0;
  else { 
    int mid = n/2;
    if (f(v,S[mid])) return binSearch(S, mid, v, f);
    else return mid + 1 + binSearch(S+mid+1, (n-mid)-1, v, f);
  }
}

template <class ET, class F> 
int binSearch(ET* S, int n, ET v, F f) {
  ET* T = S;
  while (n > 0) {
    int mid = n/2;
    if (f(v,T[mid])) n = mid;
    else {
      n = (n-mid)-1;
      T = T + mid + 1;
    }
  }
  return T-S;
}

template <class ET, class F> 
void merge(ET* S1, int l1, ET* S2, int l2, ET* R, F f) {
  int lr = l1 + l2;
  if (lr > _MERGE_BSIZE) {
    if (l2>l1)  merge(S2,l2,S1,l1,R,f);
    else {
      int m1 = l1/2;
      int m2 = binSearch(S2,l2,S1[m1],f);
      cilk_spawn merge(S1,m1,S2,m2,R,f);
      merge(S1+m1,l1-m1,S2+m2,l2-m2,R+m1+m2,f);
      cilk_sync;
    }
  } else {
    ET* pR = R; 
    ET* pS1 = S1; 
    ET* pS2 = S2;
    ET* eS1 = S1+l1; 
    ET* eS2 = S2+l2;
    while (true) {
      *pR++ = f(*pS2,*pS1) ? *pS2++ : *pS1++;
      if (pS1==eS1) {std::copy(pS2,eS2,pR); break;}
      if (pS2==eS2) {std::copy(pS1,eS1,pR); break;}
    }
  }
}
