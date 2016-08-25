#include "sequence.h"
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

  //find indices with LCP = 0
  bool* flags = newA(bool,n);
  parallel_for(uintT i=0;i<n;i++) {
    if(Rank[i] > 0) {
      uintT j = SA[Rank[i] - 1];
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
    uintT h = 0;
    for(uintT i = start; i < end;i++) {
      if(Rank[i] > 0) {
	uintT j = SA[Rank[i] - 1];
	while (s[i + h] == s[j + h]) ++h;
	lcp[Rank[i]] = h;
	if (h > 0) --h;      
      }
    }
  }
  free(offsets);
  free(Rank);
  return lcp;
}
