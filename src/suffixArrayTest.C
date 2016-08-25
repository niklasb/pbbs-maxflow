#include <iostream>
#include <cstdlib>
#include "gettime.h"
#include "sequence.h"
#include "stringGen.h"
using namespace std;

int* suffixArrayNoLCP(int* s, int n);

bool isPermutation(int *SA, int n) {
  bool *seen = new bool[n];
  for (int i = 0;  i < n;  i++) seen[i] = 0;
  for (int i = 0;  i < n;  i++) seen[SA[i]] = 1;
  for (int i = 0;  i < n;  i++) if (!seen[i]) return 0;
  return 1;
}

bool sleq(int *s1, int *s2) {
  if (s1[0] < s2[0]) return 1;
  if (s1[0] > s2[0]) return 0;
  return sleq(s1+1, s2+1);
} 

bool isSorted(int *SA, int *s, int n) {
  for (int i = 0;  i < n-1;  i++) {
    if (!sleq(s+SA[i], s+SA[i+1])) {
      cout << "not sorted at i = " << i+1 << " : " << SA[i] 
	   << "," << SA[i+1] << endl;
      return 0;
    }
  }
  return 1;  
}

int cilk_main(int argc, char **argv) {

  if (argc == 3 && argv[1][0] == 'f'){
    char* filename = (char*)argv[2];
    seq<char> str = dataGen::readCharFile(filename);
    int n = str.size();
    int* s = newA(int,n+1);
    for (int i = 0; i < n; i++) s[i] = (unsigned char) str[i];
    s[n] = 0;
    str.del();
    startTime();
    int* SA = suffixArrayNoLCP(s, n);
    stopTime(.3,"Suffix Array (from file)");
    free(s); free(SA);
    return 0;
  }

  int n = 10;
  if (argc > 1) n = std::atoi(argv[1]);

  {
    char* str = dataGen::trigramString(0, n);
    int* s = newA(int,n+1);
    for (int i = 0; i < n+1; i++) s[i] = str[i];
    free(str);
    startTime();
    int* SA = suffixArrayNoLCP(s, n);
    stopTime(.3,"Suffix Array (trigram string)");
    isPermutation(SA, n);
    isSorted(SA, s, n);
    free(s); free(SA);
  }

  {
    int* s = newA(int,n+1);
    for (int i = 0; i < n; i++) s[i] = 1;
    s[n] = 0;
    startTime();
    int* SA = suffixArrayNoLCP(s, n);
    stopTime(.1,"Suffix Array (equal string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n);  too expensive to run
    free(s); free(SA);
  }

  {
    int* s = newA(int,n+1);
    for (int i = 0; i < n; i++) s[i] = 1 + (i%32);
    s[n] = 0;
    startTime();
    int* SA = suffixArrayNoLCP(s, n);
    stopTime(.1,"Suffix Array (repeated string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n); too expensive to run
    free(s); free(SA);
  }

  reportTime("Suffix Array (weighted average)");
}
