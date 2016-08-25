#include <iostream>
#include <cstdlib>
#include "gettime.h"
#include "sequence.h"
#include "utils.h"
#include "cilk.h"

using namespace std;
int cilk_main(int argc, char **argv) {
  int n = atoi(argv[1]); int m = atoi(argv[2]);
  int* ptr = (int*) malloc(sizeof(int)*m);
  cilk_for(int i=0;i<m;i++) ptr[i] = 1;
  long* lptr = (long*) malloc(sizeof(long)*m);
  cilk_for(int i=0;i<m;i++) lptr[i] = 1;
  bool* bptr = (bool*) malloc(sizeof(bool)*m);
  cilk_for(int i=0;i<m;i++) bptr[i] = 1;

  startTime();
  cilk_for(int i=0;i<n;i++){
    if(utils::CAS(ptr+i%m,1,0)){
      ptr[i%m] = 1;
    }
  }
  nextTime("Old CAS w/ reset (int): ");
  cilk_for(int i=0;i<n;i++){
    if(__sync_bool_compare_and_swap(ptr+i%m, 1, 0)){
      ptr[i%m] = 1;
    }
  }
  nextTime("New CAS w/ reset (int): ");
  
  cilk_for(int i=0;i<n;i++){
    utils::CAS(ptr+i%m,1,0);
  }
  nextTime("Old CAS w/o reset (int): ");
  cilk_for(int i=0;i<n;i++){
    __sync_val_compare_and_swap(ptr+i%m, 1, 0);
  }
  nextTime("New CAS w/o reset (int): ");

  cilk_for(int i=0;i<n;i++){
    if(utils::CAS(lptr+i%m,1,0)){
      lptr[i%m] = 1;
    }
  }
  nextTime("Old CAS w/ reset (long): ");
  cilk_for(int i=0;i<n;i++){
    if(__sync_bool_compare_and_swap(lptr+i%m, 1, 0)){
      lptr[i%m] = 1;
    }
  }
  nextTime("New CAS w/ reset (long): ");

  cilk_for(int i=0;i<n;i++){
    utils::CAS(lptr+i%m,1,0);
  }
  nextTime("Old CAS w/o reset (long): ");
  cilk_for(int i=0;i<n;i++){
    __sync_val_compare_and_swap(lptr+i%m, 1, 0);

  }
  nextTime("New CAS w/o reset (long): ");

  cilk_for(int i=0;i<n;i++){
    if(__sync_bool_compare_and_swap(bptr+i%m, 1, 0)){
      bptr[i%m] = 1;
    }
  }
  nextTime("New CAS w/ reset (bool): ");

  cilk_for(int i=0;i<n;i++){
    __sync_val_compare_and_swap(bptr+i%m, 1, 0);
  }
  nextTime("New CAS w/o reset (bool): ");

  free(ptr); free(lptr); free(bptr);


  
  int* A = new int[n];
  int* B = new int[n];
  int* I = new int[n];
  cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
  cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
  timeStatement(cilk_for (int i=0;i<n;i++) 
		utils::CAS(&B[I[i]],0,A[i]),"Old scatter CAS (int)");
  cilk_for (int i=0;i<n;i++)  I[i] = I[i]%1000000;
  timeStatement(cilk_for (int i=0;i<n;i++) 
		utils::CAS(&B[I[i]],0,A[i]),"Old scatter CAS contended (10^6)");
  timeStatement(cilk_for (int i=0;i<n;i++) 
		if (B[I[i]] == 0) 
		  utils::CAS(&B[I[i]],0,A[i]);,
		"Old scatter CAS contended conditional (10^6)");
  cilk_for (int i=0;i<n;i++)  I[i] = I[i]%100000;
  timeStatement(cilk_for (int i=0;i<n;i++) 
		utils::CAS(&B[I[i]],0,A[i]),
		"Old scatter CAS contended (10^5)");
  timeStatement(cilk_for (int i=0;i<n;i++) 
		if (B[I[i]] == 0) 
		  utils::CAS(&B[I[i]],0,A[i]);,
		"Old scatter CAS contended conditional (10^5)");


  //start new CAS
  cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
  cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;

  timeStatement(cilk_for (int i=0;i<n;i++) 
		__sync_val_compare_and_swap(&B[I[i]],0,A[i]),"New scatter CAS (int)");
  cilk_for (int i=0;i<n;i++)  I[i] = I[i]%1000000;
  timeStatement(cilk_for (int i=0;i<n;i++) 
		 __sync_val_compare_and_swap(&B[I[i]],0,A[i]),"New scatter CAS contended (10^6)");
  timeStatement(cilk_for (int i=0;i<n;i++) 
		if (B[I[i]] == 0) 
		  __sync_val_compare_and_swap(&B[I[i]],0,A[i]);,
		"New scatter CAS contended conditional (10^6)");
  cilk_for (int i=0;i<n;i++)  I[i] = I[i]%100000;
  timeStatement(cilk_for (int i=0;i<n;i++) 
		__sync_val_compare_and_swap(&B[I[i]],0,A[i]),
		"New scatter CAS contended (10^5)");
  timeStatement(cilk_for (int i=0;i<n;i++) 
		if (B[I[i]] == 0) 
		  __sync_val_compare_and_swap(&B[I[i]],0,A[i]);,
		"New scatter CAS contended conditional (10^5)");
  free(A); free(B); free(I);


}
