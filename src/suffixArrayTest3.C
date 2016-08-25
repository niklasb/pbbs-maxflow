#include <iostream>
#include <cstdlib>
#include "gettime.h"
#include "sequence.h"
#include "stringGen.h"
using namespace std;

typedef unsigned char uchar;

int* suffixArray(uchar* s, int n);

bool isPermutation(int *SA, int n) {
  bool *seen = new bool[n];
  for (int i = 0;  i < n;  i++) seen[i] = 0;
  for (int i = 0;  i < n;  i++) seen[SA[i]] = 1;
  for (int i = 0;  i < n;  i++) 
    if (!seen[i]) {
      cout << "no reference to i = " << i << endl;
      abort();
    }
  delete [] seen;
  return 1;
}

template <class E>
bool sleq(E *s1, E *s2) {
  if (s1[0] < s2[0]) return 1;
  if (s1[0] > s2[0]) return 0;
  return sleq(s1+1, s2+1);
} 

template <class E>
bool isSorted(int *SA, E *s, int n) {
  long *hold = newA(long,n+1);
  cilk_for (int i=0; i < n; i++) hold[i] = s[i];
  hold[n] = -1;
  for (int i = 0;  i < n-1;  i++) {
    if (!sleq(hold+SA[i], hold+SA[i+1])) {
      cout << "not sorted at i = " << i << " : " << SA[i] 
	   << "," << SA[i+1] << endl;
      for (int j=0; j<20; j++) cout << (char) *(hold+SA[i]+j);
      cout << endl;
      for (int j=0; j<20; j++) cout << (char) *(hold+SA[i+1]+j);
      cout << endl;
      abort();
    }
  }
  return 1;  
}

int cilk_main(int argc, char **argv) {
  int n = 10;
  if (argc > 1) n = std::atoi(argv[1]);

  if (argc == 3 && argv[1][0] == 'f'){
    char* filename = (char*)argv[2];
    seq<char> data = dataGen::readCharFile(filename);
    uchar* str = (uchar *) data.S;
    int n = data.size();
    startTime();
    int* SA = suffixArray(str, n);
    stopTime(1.0,"Suffix Array");
    isPermutation(SA, n);
    //isSorted(SA, str, n);
    return 1;
  }

  if (1) {
    uchar* str = (uchar*) dataGen::trigramString(0, n);
    startTime();
    //for(int i=0;i<n;i++)cout<<(int)str[i]<<" ";cout<<endl;

    int* SA = suffixArray(str, n);

    stopTime(1.0,"Suffix Array (trigram string)");
    isPermutation(SA, n);
    isSorted(SA, str, n);
    free(str);
  }

  if (1) {
    uchar* s = newA(uchar,n+1);
    for (int i = 0; i < n; i++) 
      s[i] = (uchar) (dataGen::hash<int>(i) % 127 + 1);
    s[n] = 0;
    startTime();
    int* SA = suffixArray(s, n);
    stopTime(.5,"Suffix Array (random string)");
    isPermutation(SA, n);
    isSorted(SA, s, n);
    free(s);
  }

  if (1) {
    uchar* s = newA(uchar,n+1);
    for (int i = 0; i < n; i++) s[i] = 1;
    s[n] = 0;
    startTime();
    int* SA = suffixArray(s, n);
    stopTime(.1,"Suffix Array (equal string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n);  //too expensive to run
    free(s);
  }

  if (0) {
    uchar* s = newA(uchar,n);
    for (int i = 0; i < n; i++) s[i] = 1 + (i%32);
    startTime();
    int* SA = suffixArray(s, n);
    stopTime(.1,"Suffix Array (repeated string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n); too expensive to run
    free(s);
  }

  reportTime("Suffix Array (weighted average)");
}
