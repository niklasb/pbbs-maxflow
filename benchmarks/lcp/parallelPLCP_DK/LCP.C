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

  //find indices with LCP = 0
  bool* flags = newA(bool,n);
  parallel_for(uintT i=0;i<n;i++) {
    if(Phi[i] != n) {
      uintT j = Phi[i];
      flags[i] = (s[i] != s[j]);
    } else flags[i] = 0;
  }
  flags[0] = 1;

  //find offsets of chunks
  uintT* indices = newA(uintT,n);
  parallel_for(uintT i=0;i<n;i++) indices[i] = i;
  uintT* offsets = newA(uintT,n+1);
  uintT numChunks = sequence::pack(indices,offsets,flags,n);
  offsets[numChunks] = n;
  free(flags); free(indices);

  parallel_for(uintT chunk=0;chunk<numChunks;chunk++) {
    uintT start = offsets[chunk];
    uintT end = offsets[chunk+1];
    intT l = 0;
    for(uintT i = start; i < end;i++) {
      if(Phi[i] == n) l = 0; //first suffix has PLCP of 0
      else while(s[i+l] == s[Phi[i]+l]) ++l;
      plcp[i] = l; 
      l = std::max(l-1,0);
    }
  }

  free(offsets);

  uintT* lcp = newA(uintT,n);
  parallel_for(uintT i=0;i<n;i++)
    lcp[i] = plcp[SA[i]];

  free(plcp);
  return lcp;
}
