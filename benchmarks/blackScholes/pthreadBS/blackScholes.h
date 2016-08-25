// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <math.h>
#include "options.h"
#include <pthread.h>

using namespace std;

// Multi-threaded pthreads header
#define ENABLE_THREADS
/* // Add the following line so that icc 9.0 is compatible with pthread lib. */
/* #define __thread __threadp */
/* MAIN_ENV */
/* #undef __thread */

int bs_thread(void *tid_ptr) {

  int i, j;
  fptype price;
  fptype priceDelta;
  int tid = *(int *)tid_ptr;
  int start = tid * (numOptions / nThreads);
  int end = start + (numOptions / nThreads);

  for (j=0; j<NUM_RUNS; j++) {
    for (i=start; i<end; i++) {
      /* Calling main function to calculate option value based on 
       * Black & Scholes's equation.
       */
      price = BlkSchlsEqEuroNoDiv( sptprice[i], strike[i],
				   rate[i], volatility[i], otime[i], 
				   otype[i], 0);
      prices[i] = price;

    }
  }
  return 0;
}
