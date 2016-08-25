#include <iostream>
#include <iomanip>
#include "gettime.h"
#include "utils.h"
#include "graphGen.h"
#include "cilk.h"
#include <math.h>
using namespace std;

// multiply a compresses sparse row matrix
template <class E, class FM, class FA>
void MvMult(sparseRowMajor<E> A, E* v, E* o, FM fm, FA fa) {
  int n = A.numRows;
  int* starts = A.Starts; int *cols = A.ColIds; E* vals = A.Values;
  cilk_for (int i=0; i < n; i++) {
    int s = starts[i];
    int e = starts[i+1];
    E sum = 0;
    int* cp = cols+s;
    int* ce = cols+e;
    E* vp = vals+s; 
    while (cp < ce-1) {
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
    }
    if (cp < ce) 
      sum += v[*cp]*(*vp);
    o[i] = sum;
  }
}
