#include "parallel.h"
#include "utils.h"

using namespace std;

uintT* LCP(unsigned char* s, uintT* SA, long n) {
  uintT* Phi = newA(uintT,n);
  uintT* plcp = Phi;
  for(uintT i=1;i<n;i++)
    Phi[SA[i]] = SA[i-1];
  Phi[SA[0]] = n;//-1;

  uintT l = 0;
  for(uintT i=0;i<n;i++) {
    if(Phi[i] == n) l = 0; //first suffix has PLCP of 0
    else while(s[i+l] == s[Phi[i]+l]) ++l;
    plcp[i] = l; 
    if(l > 0) l--;
  }

  uintT* lcp = newA(uintT,n);
  for(uintT i=0;i<n;i++)
    lcp[i] = plcp[SA[i]];

  free(plcp);
  return lcp;
}
