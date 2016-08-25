#include "gettime.h"
#include "hash.h"
#include "seq.h"
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
    seq<int> r = removeDuplicates(seq<int>(A,n));
    free(A); r.del();
  }

  {
    int *A = dataGen::randIntRange(0,n,n);
    startTime();
    seq<int> r = removeDuplicates(seq<int>(A,n));
    stopTime(.1,"Remove Duplicates (int, random in n)");
    free(A); r.del();
  }

  {
    int *A = dataGen::expDist<int>(0,n);
    startTime();
    seq<int> r = removeDuplicates(seq<int>(A,n));
    stopTime(.1,"Remove Duplicates (int, exponential)");
    free(A); r.del();  
  }

  {
    char **A = dataGen::trigramWords(0,n);
    startTime();
    seq<char*> r = removeDuplicates(seq<char*>(A,n));
    stopTime(.2,"Remove Duplicates (trigram strings)");
    dataGen::freeWords(A,n);
    r.del();
  }

  reportTime("Remove Duplicates (weighted average)");

}
