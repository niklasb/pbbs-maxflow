#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include <cmath>
using namespace std;

uintT* LCP(unsigned char* s, uintT* SA, long n) { 
  uintT *lcp = newA(uintT,n);

  lcp[0] = 0;

  parallel_for(uintT i = 1; i < n; i++) {
    uintT h = 0;
    uintT j = SA[i];
    uintT k = SA[i-1];
    while (s[j + h] == s[k + h]) ++h;
    lcp[i] = h;
  }

  return lcp;
}
