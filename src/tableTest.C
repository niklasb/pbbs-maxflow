#include "gettime.h"
#include "hash.h"
#include "sequence.h"
#include "seqGen.h"
#include <algorithm>
#include <iostream>
using namespace std;

int cilk_main(int argc, char* argv[]) {
  int n = 10;
  if (argc > 1) n = std::atoi(argv[1]);

  // do once to touch memory
  {
    int *A = dataGen::randIntRange(0,n,n);
    int *B = dataGen::randIntRange(n,2*n,n);
    IntTable it = makeIntTable(2*n/3);
    cilk_for(int i=0;i<n;i++) it.insert(A[i]);
    cilk_for(int i=0;i<n;i++) it.find(B[i]);
    free(A); free(B); it.del();
  }

  {
    int *A = dataGen::randIntRange(0,n,n);
    int *B = dataGen::randIntRange(n,2*n,n);
    int *C = newA(int,n);
    startTime();
    IntTable it = makeIntTable(2*n/3);
    cilk_for(int i=0;i<n;i++) it.insert(A[i]);
    cilk_for(int i=0;i<n;i++) C[i] = it.find(B[i]);
    stopTime(.1, "Table (random in n, insert and find)");
    free(A); free(B); free(C); it.del();
  }

  {
    char **A = dataGen::trigramWords(0,n);
    char **B = dataGen::trigramWords(n,2*n);
    char **C = newA(char*,n);
    startTime();
    StrTable it = makeStrTable(n/3);
    cilk_for(int i=0;i<n;i++) it.insert(A[i]);
    cilk_for(int i=0;i<n;i++) C[i] = it.find(B[i]);
    stopTime(.2,"Table (trigram words, insert and find)");
    free(A); free(B); free(C); it.del();
  }

  reportTime("Table (weighted average)");
}
