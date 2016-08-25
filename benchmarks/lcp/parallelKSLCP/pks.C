//by Julian Shun. Modified PBBS suffix array code to compute LCPs
//given suffix array as input

#include <iostream>
#include "sequence.h"
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "rangeMin.h"
using namespace std;

inline bool leq(uintT a1, uintT a2,   uintT b1, uintT b2) {
  return(a1 < b1 || a1 == b1 && a2 <= b2); 
}                                  

inline bool leq(uintT a1, uintT a2, uintT a3, uintT b1, uintT b2, uintT b3) {
  return(a1 < b1 || a1 == b1 && leq(a2, a3, b2, b3)); 
}

struct compS {
  uintT* _s;
  uintT* _s12;
  compS(uintT* s, uintT* s12) : _s(s), _s12(s12) {}
  bool operator () (long i, long j) {
    if (i%3 == 1 || j%3 == 1) 
      return leq(_s[i],_s12[i+1], _s[j],_s12[j+1]);
    else
      return leq(_s[i],_s[i+1],_s12[i+2], _s[j],_s[j+1],_s12[j+2]);
  }
};

inline long computeLCP(uintT* LCP12, uintT* rank, myRMQ & RMQ, 
		      long j, long k, uintT* s, long n){
 
  long rank_j=rank[j];
  long rank_k=rank[k];
  if(rank_j > rank_k) {swap(rank_j,rank_k);} //swap for RMQ query

  long l = ((rank_j == rank_k-1) ? LCP12[rank_j] 
	   : LCP12[RMQ.query(rank_j,rank_k-1)]);
  long lll = 3*l;
  if (s[j+lll] == s[k+lll]) {
    if (s[j+lll+1] == s[k+lll+1]) return lll + 2;
    else return lll + 1;
  } 
  return lll;
}

// This recursive version requires s[n]=s[n+1]=s[n+2] = 0
uintT* LCPrec(uintT* s, uintT* SA, long n) {
  long n0=(n+2)/3, n1=(n+1)/3, n12=n-n0;

  //get sorted12
  uintT* flags = newA(uintT,n);
  parallel_for(long i=0;i<n;i++) 
    flags[i] = (SA[i] % 3 != 0); 
  
  uintT maxFlag = sequence::plusScan<uintT>(flags,flags,n);

  uintT* sorted12 = newA(uintT,n12);
  parallel_for(long i=0;i<n;i++) {
    if(SA[i] % 3 != 0) sorted12[flags[i]] = SA[i];
  } 

  //get ranks of SA
  //flags store new ranks also
  uintT* Rank = newA(uintT,n);

  parallel_for(long i=0;i<n;i++) if(SA[i] % 3) { 
    Rank[SA[i]] = flags[i]; 
  }
  free(flags);

  // generate names based on 3 chars
  uintT* name12 = newA(uintT,n12);
  parallel_for (long i = 1;  i < n12;  i++) {
    if (s[sorted12[i]] != s[sorted12[i-1]] 
	|| s[sorted12[i]+1] != s[sorted12[i-1]+1] 
	|| s[sorted12[i]+2] != s[sorted12[i-1]+2]) 
      name12[i] = 1;
    else name12[i] = 0;
  }
  
  name12[0] = 1;
  sequence::scanI(name12, name12, n12, utils::addF<uintT>(), (uintT)0);
  long names = name12[n12-1];

  uintT* SA12;
  uintT* LCP12;
  // recurse if names are not yet unique
  if (names < n12) {
    uintT* s12  = newA(uintT, n12 + 3);  
    s12[n12] = s12[n12+1] = s12[n12+2] = 0; 

    SA12 = newA(uintT,n12);

    // move mod 1 suffixes to bottom half and and mod 2 suffixes to top
    // create SA12 based on sorted12 since it came from SA
    parallel_for (long i= 0; i < n12; i++)
      if (sorted12[i]%3 == 1) {
	s12[sorted12[i]/3] = name12[i];
	SA12[i] = sorted12[i]/3;
      }
      else {
	s12[sorted12[i]/3+n1] = name12[i];
	SA12[i] = sorted12[i]/3+n1;
      }

    free(name12); 
    free(sorted12);
    LCP12 = LCPrec(s12, SA12, n12);

    free(s12);

    // restore proper indices into original array
    parallel_for (long i = 0;  i < n12;  i++) {
      long l = SA12[i]; 
      SA12[i] = (l<n1) ? 3*l+1 : 3*(l-n1)+2;
    }
  } else {
    free(name12); // names not needed if we don't recurse
    free(sorted12);
    SA12 = newA(uintT,n12);

    parallel_for (long i=0;i<n12;i++) {
      long j = i/2;
      long l = Rank[3*j+1+i%2];
      SA12[i] = (l<n1) ? 3*l+1 : 3*(l-n1)+2;
    }

    LCP12 = newA(uintT, n12+3);
    parallel_for(long i=0; i<n12+3; i++) 
      LCP12[i] = 0; //LCP's are all 0 if not recursing
  }

  free(SA12);
  //get LCPs
  uintT* LCP = newA(uintT, n);  
  LCP[n-2] = LCP[n-1] = 0;

  myRMQ RMQ(LCP12, n12+3); //simple rmq
  parallel_for(long i=0;i<n-1;i++){ 
    long j = SA[i];
    long k = SA[i+1];
    int CLEN = 16;
    long ii;
    for (ii=0; ii < CLEN; ii++) 
      if (s[j+ii] != s[k+ii]) break;

    if (ii != CLEN) LCP[i] = ii;
    else {
      if (j%3 != 0 && k%3 != 0)  {
	LCP[i] = computeLCP(LCP12, Rank, RMQ, j, k, s, n);
      } 
      else if (j%3 != 2 && k%3 != 2) {
	LCP[i] = 1 + computeLCP(LCP12, Rank, RMQ, j+1, k+1, s, n);
      }
      else { 
	LCP[i] = 2 + computeLCP(LCP12, Rank, RMQ, j+2, k+2, s, n);
      }
    }
  }

  free(LCP12);
  free(Rank);
  return LCP;
}

uintT* LCP(unsigned char* s, uintT* SA, long n) {
  uintT *ss = newA(uintT, n+3); 
  ss[n] = ss[n+1] = ss[n+2] = 0;
  parallel_for (long i=0; i < n; i++) ss[i] = ((uintT) s[i])+1;

  uintT *lcp =  LCPrec(ss, SA, n);
  free(ss);
  return lcp;
}
