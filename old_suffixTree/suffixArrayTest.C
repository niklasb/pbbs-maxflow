#include <iostream>
#include <cstdlib>
#include "gettime.h"
#include "sequence.h"
#include "stringGen.h"
#include "stackSpace.h"
using namespace std;

intT* suffixArrayNoLCP(intT* s, intT n, stackSpace* stack);
pair<intT*,intT*> suffixArray(intT* s, intT n, bool findLCP, stackSpace* stack);

bool isPermutation(intT *SA, intT n) {
  bool *seen = new bool[n];
  for (intT i = 0;  i < n;  i++) seen[i] = 0;
  for (intT i = 0;  i < n;  i++) seen[SA[i]] = 1;
  for (intT i = 0;  i < n;  i++) if (!seen[i]) return 0;
  return 1;
}

bool sleq(intT *s1, intT *s2) {
  if (s1[0] < s2[0]) return 1;
  if (s1[0] > s2[0]) return 0;
  return sleq(s1+1, s2+1);
} 

bool isSorted(intT *SA, intT *s, intT n) {
  for (intT i = 0;  i < n-1;  i++) {
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
    intT n = str.size();
    stackSpace* stack = new stackSpace(1024+4.5*sizeof(intT)*n);
    intT* s = newA(intT,n+1);
    for (intT i = 0; i < n; i++) s[i] = (unsigned char) str[i];
    s[n] = 0;
    str.del();
    startTime();
    intT* SA = suffixArrayNoLCP(s, n, stack);
    stopTime(.3,"Suffix Array (from file)");
    free(s); 
    stack->pop(1);
    return 0;
  }

  intT n = 10;
  if (argc > 1) n = std::atoi(argv[1]);

  // the space needed by the suffix Array code
  stackSpace* stack = new stackSpace(1024+4.5*sizeof(intT)*n);

  {
    char* str = dataGen::trigramString(0, n);
    intT* s = newA(intT,n+1);
    for (intT i = 0; i < n+1; i++) s[i] = str[i];
    free(str);
    startTime();
    intT* SA = suffixArrayNoLCP(s, n, stack);
    stopTime(.3,"Suffix Array (trigram string)");
    isPermutation(SA, n);
    isSorted(SA, s, n);
    free(s); 
    stack->pop(1);
  }

  {
    intT* s = newA(intT,n+1);
    for (intT i = 0; i < n; i++) s[i] = 1;
    s[n] = 0;
    startTime();
    intT* SA = suffixArrayNoLCP(s, n, stack);
    stopTime(.1,"Suffix Array (equal string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n);  too expensive to run
    free(s); 
    stack->pop(1);
  }

  {
    intT* s = newA(intT,n+1);
    for (intT i = 0; i < n; i++) s[i] = 1 + (i%32);
    s[n] = 0;
    startTime();
    intT* SA = suffixArrayNoLCP(s, n, stack);
    stopTime(.1,"Suffix Array (repeated string)");
    isPermutation(SA, n);
    //isSorted(SA, s, n); too expensive to run
    free(s); 
    stack->pop(1);
  }

  if (!stack->isEmpty()) 
    cout << "memory leak" << endl;

  reportTime("Suffix Array (weighted average)");
}
