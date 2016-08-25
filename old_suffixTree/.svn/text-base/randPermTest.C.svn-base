#include "gettime.h"
#include "randPerm.h"
#include "seqGen.h"
#include <iostream>
using namespace std;

int cilk_main(int argc, char* argv[]) {
  int n = 10;
  if (argc > 1) n = std::atoi(argv[1]);

  // Do once to touch memory
  {
    int *A = newA(int,n);
    randPerm(A,n);
    free(A); 
  }

  {
    int *A = newA(int,n);
    startTime();
    randPerm(A,n);
    stopTime(.1, "Random Permutation (int)");
    free(A); 
  }

  {
    dataGen::payload* A = dataGen::randPayload(0,n);
    startTime();
    randPerm(A,n);
    stopTime(.1, "Random Permutation (4 x double)");
    free(A);
  }

  reportTime("Random Permutation (weighted average)");
}

