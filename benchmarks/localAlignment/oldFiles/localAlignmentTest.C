#include <iostream>
#include "cilk.h"
#include "gettime.h"
#include "utils.h"
using namespace std;

int editDistance(char* A, int n, char* B, int m);
int editDistanceLinearSpace(char* A, int n, char* B, int m);
int editDistanceCacheEfficient(char* A, int n, char* B, int m);

int cilk_main(int argc, char** argv){
  char* alphabet = "acgt";
  int n = 10;
  int m = 20;
  if(argc == 3){ n = atoi(argv[1]); m = atoi(argv[2]); }
  char *A = new char[n];
  char *B = new char[m];
  
  startTime();
  //random strings
  cilk_for(int i=0;i<n;i++){
    int k = utils::hash(i)%4;
    A[i] = alphabet[k];
  }
  cilk_for(int i=0;i<m;i++){
    int k = utils::hash(i+n)%4;
    B[i] = alphabet[k];
  }
  nextTime("Time for random strings");
  int ed = editDistance(A,n,B,m);
  nextTime("Time for edit distance");
  cout << "n= " << n << " m= " << m << 
    endl << "edit distance: " << ed << endl;
  //ed = editDistanceCacheEfficient(A,n,B,m);
  //nextTime("Time for cache-efficient edit distance");
  //cout << "n= " << n << " m= " << m <<
  // endl << "edit distance: " << ed << endl;
  ed = editDistanceLinearSpace(A,n,B,m);
  nextTime("Time for space-efficient edit distance");
  cout << "n= " << n << " m= " << m << 
    endl << "edit distance: " << ed << endl;
}
