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

#define ENABLE_TBB

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

using namespace tbb;

struct mainWork {
  mainWork() {}
  mainWork(mainWork &w, tbb::split) {}

  void operator()(const tbb::blocked_range<int> &range) const {
    fptype price;
    int begin = range.begin();
    int end = range.end();

    for (int i=begin; i!=end; i++) {
      /* Calling main function to calculate option value based on
       * Black & Scholes's equation.
       */

      price = BlkSchlsEqEuroNoDiv( sptprice[i], strike[i],
                                   rate[i], volatility[i], otime[i],
                                   otype[i], 0);
      prices[i] = price;

    }
  }
};


int bs_thread(void *tid_ptr) {
    int j;
    tbb::affinity_partitioner a;

    mainWork doall;
    for (j=0; j<NUM_RUNS; j++) {
      tbb::parallel_for(tbb::blocked_range<int>(0, numOptions), doall, a);
    }

    return 0;
}
