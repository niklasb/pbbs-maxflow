#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include <cmath>
using namespace std;

uintT* LCP(unsigned char* s, uintT* SA, long n) { 
  uintT *Rank = newA(uintT,n);
  uintT *lcp = newA(uintT,n);

  lcp[0] = 0;
  parallel_for (uintT i = 0; i < n; i++) Rank[SA[i]] = i;
  uintT p = getWorkers();
  
  uintT chunkSize = n/p;
  uintT numChunks = ceil((double)n/chunkSize);

  parallel_for(uintT chunk=0;chunk<numChunks;chunk++) {
    uintT h = 0;
    for(uintT i = chunk*chunkSize; i < min((chunk+1)*chunkSize,(uintT)n);i++) {
      if(Rank[i] > 0) {
	uintT j = SA[Rank[i] - 1];
	while (s[i + h] == s[j + h]) ++h;
	lcp[Rank[i]] = h;
	if (h > 0) --h;      
      }
    }
  }

  free(Rank);

  return lcp;
}
