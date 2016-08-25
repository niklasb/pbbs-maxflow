#include <iostream>
#include <algorithm>
#include "gettime.h"
#include "utils.h"
#include "randPerm.h"
#include "cilk.h"
#include "seqGen.h"
using namespace std;

struct strCmp {
  bool operator() (char* s1c, char* s2c) {
    char* s1 = s1c, *s2 = s2c;
    while (*s1 && *s1==*s2) {s1++; s2++;};
    return (*s1 < *s2);
  }
};

template <class E, class BinPred>
void checkSorted(E* A, int n, BinPred f) {
  cilk_for (int i = 0; i < n-1; ++i) 
    if (f(A[i+1],A[i])) {
      std::cout << "Sort failed at location: " << i << "," << i+1 << std::endl;
      abort();
    }
}

int cilk_main(int argc, char* argv[]) {
  int n = 10, cnt = 8;
  if (argc > 1) n = std::atoi(argv[1]);

 {
  {
    double *A = dataGen::rand<double>(0,n);
    startTime();
    compSort(A, n, less<double>());
    stopTime(.1,"Comparison Sort (double, random)");
    checkSorted(A,n,less<double>());
    free(A);
  }

  {
    double *A = dataGen::expDist<double>(0,n);
    startTime();
    compSort(A, n, less<double>());
    stopTime(.1,"Comparison Sort (double, exponential)");
    checkSorted(A,n,less<double>());
    free(A);
  }

  {
    double *A = dataGen::almostSorted<double>(0,n);
    startTime();
    compSort(A, n, less<double>());
    stopTime(.1,"Comparison Sort (double, almost sorted)");
    checkSorted(A,n,less<double>());
    delete[] A;
  }

  {
    char **A = dataGen::trigramWords(0,n);
    startTime();
    compSort(A, n, strCmp());
    stopTime(.2,"Comparison Sort (trigram strings adjacent)");
    checkSorted(A,n, strCmp());
    dataGen::freeWords(A,n);
  }

  {
    char **A = dataGen::trigramWords(0,n);
    //random_shuffle(A,A+n);
    randPerm(A,n);
    startTime();
    compSort(A, n, strCmp());
    stopTime(.2,"Comparison Sort (trigram strings, random layout)");
    checkSorted(A,n, strCmp());
    dataGen::freeWords(A,n);
  }

  {
    dataGen::payload* A = dataGen::randPayload(0,n);
    startTime();
    compSort(A, n, dataGen::payloadCmp());
    stopTime(.3, "Comparison Sort (double with 3 x double payload)");
    checkSorted(A,n, dataGen::payloadCmp());
    free(A);
  }

  reportTime("Comparison Sort (weighted average)");
  }
}
