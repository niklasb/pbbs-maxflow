#include "parallel.h"
#include "utils.h"

uintT* LCP(unsigned char* s, uintT* SA, long n) { 
  uintT h = 0;
  uintT *Rank = newA(uintT,n);
  uintT *lcp = newA(uintT,n);

  lcp[0] = 0;
  for (uintT i = 0; i < n; i++) Rank[SA[i]] = i;
  for (uintT i = 0; i < n; i++) {
    if (Rank[i] > 0) {
      uintT j = SA[Rank[i] - 1];
      while (s[i + h] == s[j + h]) ++h;
      lcp[Rank[i]] = h;
      if (h > 0) --h;
    }
  }
  free(Rank);
  //for(int i=0;i<n;i++) std::cout << lcp[i] << " ";std::cout<<std::endl;
  return lcp;
}
