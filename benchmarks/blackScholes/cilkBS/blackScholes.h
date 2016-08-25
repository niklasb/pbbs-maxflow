// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <math.h>
#include "options.h"

using namespace std;

#define ENABLE_CILK
#include <cilk/cilk.h>

int bs_thread(void *tid_ptr) {

  int j;
  fptype price;
  fptype priceDelta;
  int tid = *(int *)tid_ptr;
  int start = tid * (numOptions / nThreads);
  int end = start + (numOptions / nThreads);

  for (j=0; j<NUM_RUNS; j++) {

    cilk_for (int i=0; i<numOptions; i++) {
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
