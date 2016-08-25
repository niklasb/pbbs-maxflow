#include "sequence.h"
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include <cmath>

using namespace std;

uintT* LCP(unsigned char* s, uintT* SA, long n) {
  uintT* Phi = newA(uintT,n);
  uintT* plcp = Phi;
  parallel_for(uintT i=1;i<n;i++)
    Phi[SA[i]] = SA[i-1];
  Phi[SA[0]] = n;

  uintT p = getWorkers();

  uintT chunkSize = n/p;
  uintT numChunks = ceil((double)n/chunkSize);

  parallel_for(uintT chunk=0;chunk<numChunks;chunk++) {
    uintT l = 0;
    for(uintT i = chunk*chunkSize; i < min((chunk+1)*chunkSize,(uintT)n);i++) {
      if(Phi[i] == n) l = 0; //first suffix has PLCP of 0
      else while(s[i+l] == s[Phi[i]+l]) ++l;
      plcp[i] = l; 
      if(l > 0) l--;
    }
  }

  uintT* lcp = newA(uintT,n);
  parallel_for(uintT i=0;i<n;i++)
    lcp[i] = plcp[SA[i]];

  free(plcp);
  return lcp;
}
