#include <iostream>
#include <algorithm>
#include "gettime.h"
#include "sequence.h"
#include "intSort.h"
#include "quickSort.h"
#include "cilk.h"
using namespace std;

#define printInfo
#define qSort

#ifdef printInfo
#define nextTimeM(_str) nextTime(_str)
#else
#define nextTimeM(_str) 
#endif

typedef unsigned char uchar;

struct chunk {
  unsigned long firstSecond;
  unsigned int third;
  unsigned int pos;
  uint pos;
};

struct chunkCmp {
  bool operator() (chunk s1c, chunk s2c) {
    if (s1c.first < s2c.first) return 1;
    else if (s1c.first == s2c.first) {
      if (s1c.second < s2c.second) return 1;
      else if (s1c.second == s2c.second) return (s1c.third < s2c.third);
    } else return 0;
  }
};

struct chunk_first {uint operator() (chunk a) {return a.first;} };
struct chunk_second {uint operator() (chunk a) {return a.second;} };

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
  #ifdef qSort
  cout << "Quicksort with 96-bit structs" <<endl;
  #else
  cout << "Radix sort"<<endl;
  #endif
  cout << "m = " << m << endl;
  #endif

  int bits = max(1,utils::log2(m+1));
  int nchars = 32/bits;
  
  cout << "bits = " << bits << endl;
  cout << "nchars = " << nchars << endl;

  cilk_for (int i=0; i < n-2*nchars; i++) {
    C[i].first = grabChars<uint>(s+i,bits,nchars);
    C[i].second = grabChars<uint>(s+i+nchars,bits,nchars);
    C[i].second = grabChars(s+i+nchars,bits,nchars); 
    C[i].pos = i;
  }

  cilk_for (int i=max(n-2*nchars,0); i < n-nchars; i++) {
    C[i].first = grabChars<uint>(s+i,bits,nchars);
    C[i].second = grabCharsEnd<uint>(s+i+nchars,bits,nchars,n-i-nchars);
    C[i].pos = i;
  }

  cilk_for (int i=max(n-nchars,0); i < n; i++) {
    C[i].first = grabCharsEnd<uint>(s+i,bits,nchars,n-i); 
    C[i].pos = i;
    }

  free(s);

  startTime();
  #ifdef qSort
  quickSort(C,n,chunkCmp());
  #else
  intSort::iSort(C,n,((ulong) 1) << nchars*bits, chunk_second());
  intSort::iSort(C,n,((ulong) 1) << nchars*bits, chunk_first());
  #endif
  nextTime("Round 0 Sort time:");

  int* names = newA(int,n);
  uint* inv_names = newA(uint,n);
  int round = 1;
  int nKeys = n;
  int offset = nchars << 1;

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

    cout<<"round = " << round << " string length = " 
	<< 2*offset << " nKeys = " << nKeys << endl;

    //create inverse names data structure
    cilk_for(uint i=0;i<n;i++) inv_names[C[i].pos] = names[i];
    
    //make chunks
    cilk_for(int i=0;i<n;i++){
      C[i].first = 1+inv_names[C[i].pos];
      C[i].second = (C[i].pos < n-offset) 
	? 1+inv_names[C[i].pos+offset] : 0;
    }
    
    /*
    //make chunks
    cilk_for(int i=0;i<n-offset;i++){
      C[i].first = 1+inv_names[i];
      C[i].second = 1+inv_names[i+offset];
      C[i].pos = i;
    }
    cilk_for(int i=max(0,n-offset);i<n;i++){
      C[i].first = 1+inv_names[i];
      C[i].second = 0;
      C[i].pos = i;
      }*/

    //sort
    #ifdef qSort
    quickSort(C,n,chunkCmp());
    #else
    intSort::iSort(C,n, n+1, chunk_second());
    intSort::iSort(C,n, n+1, chunk_first());
    #endif
    nextTime("Sort time:");

    //count unique keys
    names[n-1] = 0;
    cilk_for(int i=1;i<n;i++){
      if((C[i].first == C[i-1].first) && 
	 (C[i].second == C[i-1].second)) 
	names[i-1] = 0;
      else names[i-1] = 1;
    }

    round++; 
    offset = offset << 1;
   
  }
  //get ranks of suffixes
  int* ranks = newA(int,n);
  //do in separate function because cilk is broken
  getRanks(ranks,C,n); 
  free(names); free(inv_names); free(C);
  return ranks;
}
