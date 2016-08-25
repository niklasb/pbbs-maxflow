#include <iostream>
#include <algorithm>
#include "gettime.h"
#include "sequence.h"
#include "intSort.h"
#include "quickSort.h"
#include "cilk.h"
using namespace std;

#define printInfo

#ifdef printInfo
#define nextTimeM(_str) nextTime(_str)
#else
#define nextTimeM(_str) 
#endif

typedef unsigned char uchar;

struct chunk {
  ulong first;
  uint second;
  uint pos;
};

struct chunkCmp {
  bool operator() (chunk s1c, chunk s2c) {
    return (s1c.first < s2c.first) ? 1 : 
      ((s1c.first > s2c.first) ? 0 : (s1c.second < s2c.second));
  }
};

template <class Type>
inline Type grabChars(uchar *s, int bits, int nChars) {
  Type r = s[0];
  for (uint i=1; i < nChars; i++) r = r<<bits | s[i];
  return r; 
}
template <class Type>
inline Type grabCharsEnd(uchar *s, int bits, int nChars, int end) {
  Type r = s[0];
  for (uint i=1; i < nChars; i++)
    r = r<<bits | ((i < end) ? s[i] : 0);
  return r; 
}

void getRanks(int* ranks, chunk* C, int n){
  cilk_for (uint i=0; i < n; i++) ranks[i] = C[i].pos;
}

int* suffixArray(uchar* ss, int n) {
  if (n <= 0) cilk_spawn printf("ouch");
  chunk *C = newA(chunk,n);
  uchar *s = newA(uchar,n);

  int flags[256];
  for (uint i=0; i < 256; i++) flags[i] = 0;
  cilk_for (uint i=0; i < n; i++) 
    if (!flags[ss[i]]) flags[ss[i]] = 1;

  int m = sequence::scan(flags,flags,256,utils::addF<int>(),0);
  cilk_for (uint i=0; i < n; i++) 
    s[i] = 1+flags[ss[i]];

  #ifdef printInfo
  cout << "Quicksort with 128-bit structs" << endl;
  cout << "m = " << m << endl;
  #endif

  int bits = max(1,utils::log2(m+1));
  int nchars = 32/bits;
  int nchars_long = 64/bits;

  cout << "bits = " << bits << endl;
  cout << "nchars = " << nchars << endl;
  
  cilk_for(int i=0;i<n-nchars_long-nchars;i++){
    C[i].first = grabChars<ulong>(s+i,bits,nchars_long);
    C[i].second = grabChars<uint>(s+i+nchars_long,bits,nchars);
    C[i].pos = i;
  }

  cilk_for(int i=max(n-nchars_long-nchars,0);i< n-nchars_long;i++){
    C[i].first = grabChars<ulong>(s+i,bits,nchars_long);
    C[i].second = 
      grabCharsEnd<uint>(s+i+nchars_long,bits,nchars,n-i-nchars_long);
    C[i].pos = i;
  }
  
  cilk_for(int i=max(n-nchars_long,0);i < n;i++){
    C[i].first = grabCharsEnd<ulong>(s+i,bits,nchars_long,n-i);
    C[i].second = 0;
    C[i].pos = i;
  } 

  free(s);
  startTime();
  quickSort(C,n,chunkCmp());
  //nextTime("Round 0 sort time:");
  int* names = newA(int,n);
  uint* inv_names = newA(uint,n);
  int round = 1;
  int nKeys = n;
  int offset = nchars + nchars_long;

  while(1){ 

   //count unique keys
    names[n-1] = 0;
    cilk_for(uint i=1;i<n;i++){
      if((C[i].first == C[i-1].first) && 
	 (C[i].second == C[i-1].second)) 
	names[i-1] = 0;
      else names[i-1] = 1;
    }

    //count number of unique keys and assign names
    nKeys = 1+sequence::scan(names,names,n,utils::addF<int>(),0);

    //finish if all keys are unique
    if(nKeys == n) { 
      break;
    }
    /*
    cout<<"round = " << round << " string length = " 
	<< 3*offset << " nKeys = " << nKeys << endl;
    */
    //create inverse names data structure
    cilk_for(uint i=0;i<n;i++) inv_names[C[i].pos] = names[i];
    
    
    //make chunks
    cilk_for(int i=0;i<n;i++){
      int k = C[i].pos;
      C[i].first = (k < n-offset) ? 
	((((ulong)1+inv_names[k]) << 32) | 
	 (1+inv_names[k+offset])) :
	(((ulong)1+inv_names[k]) << 32);
      C[i].second = (k < n-2*offset) ?
	1+inv_names[k+2*offset] : 0;
    }

    /*
    //make chunks
    cilk_for(int i=0;i<n-2*offset;i++){
      C[i].first = (((ulong) 1+inv_names[i]) << 32) | 
		     (1+inv_names[i+offset]);
      C[i].second = 1+inv_names[i+2*offset];
      C[i].pos = i;
    }

    cilk_for(int i=max(0,n-2*offset);i<n-offset;i++){
      C[i].first = (((ulong)1+inv_names[i]) << 32) | 
	1+inv_names[i+offset];
      C[i].second = 0;
      C[i].pos = i;
    }
    cilk_for(int i=max(0,n-offset);i<n;i++){
      C[i].first = ((ulong) 1+inv_names[i]) << 32;
      C[i].second = 0;
      C[i].pos = i;
      }
    */

   //sort
    quickSort(C,n,chunkCmp());
    //nextTime("Sort time:");
    
    round++; 
    offset = offset * 3;
   
  }
  //get ranks of suffixes
  int* ranks = newA(int,n);
  //do in separate function because cilk is broken
  getRanks(ranks,C,n); 
  free(names); free(inv_names); free(C);
  return ranks;
}
