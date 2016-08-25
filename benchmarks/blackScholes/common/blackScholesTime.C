// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <iostream>
#include <algorithm>
#include "gettime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blackScholes.h"
#include "options.h"

using namespace std;

int main(int argc, char* argv[]) {
  FILE *file;
  int i;
  int loopnum;
  fptype * buffer;
  int * buffer2;
  int rv;

  if (argc != 4)
    {
      printf("Usage:\n\t%s <nthreads> <inputFile> <outputFile>\n", argv[0]);
      exit(1);
    }
  nThreads = atoi(argv[1]);
  char *inputFile = argv[2];
  char *outputFile = argv[3];

  //Read input data from file
  file = fopen(inputFile, "r");
  if(file == NULL) {
    printf("ERROR: Unable to open file `%s'.\n", inputFile);
    exit(1);
  }
  rv = fscanf(file, "%i", &numOptions);
  if(rv != 1) {
    printf("ERROR: Unable to read from file `%s'.\n", inputFile);
    fclose(file);
    exit(1);
  }
  if(nThreads > numOptions) {
    printf("WARNING: Not enough work, reducing number of threads to match number of options.\n");
    nThreads = numOptions;
  }

#if !defined(ENABLE_THREADS) && !defined(ENABLE_OPENMP) && !defined(ENABLE_TBB) && !defined(ENABLE_CILK)
  if(nThreads != 1) {
    printf("Error: <nthreads> must be 1 (serial version)\n");
    exit(1);
  }
#endif

  // alloc spaces for the option data
  data = (OptionData*)malloc(numOptions*sizeof(OptionData));
  prices = (fptype*)malloc(numOptions*sizeof(fptype));
  for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
      rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq, &data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
      if(rv != 9) {
	printf("ERROR: Unable to read from file `%s'.\n", inputFile);
	fclose(file);
	exit(1);
      }
    }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file `%s'.\n", inputFile);
    exit(1);
  }

  // #ifdef ENABLE_THREADS
  //     MAIN_INITENV(,8000000,nThreads);
  // #endif
  printf("Num of Options: %d\n", numOptions);
  printf("Num of Runs: %d\n", NUM_RUNS);

#define PAD 256
#define LINESIZE 64

  buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
  sptprice = (fptype *) (((unsigned long long)buffer + PAD) & ~(LINESIZE - 1));
  strike = sptprice + numOptions;
  rate = strike + numOptions;
  volatility = rate + numOptions;
  otime = volatility + numOptions;

  buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
  otype = (int *) (((unsigned long long)buffer2 + PAD) & ~(LINESIZE - 1));

  for (i=0; i<numOptions; i++) {
    otype[i]      = (data[i].OptionType == 'P') ? 1 : 0;
    sptprice[i]   = data[i].s;
    strike[i]     = data[i].strike;
    rate[i]       = data[i].r;
    volatility[i] = data[i].v;    
    otime[i]      = data[i].t;
  }

  printf("Size of data: %lu\n", numOptions * (sizeof(OptionData) + sizeof(int)));

  startTime();

#ifdef ENABLE_THREADS
  int tids[nThreads];
  pthread_t      *threads;
  pthread_attr_t  pthread_custom_attr;

  threads = (pthread_t *) malloc(nThreads * sizeof(pthread_t));
  pthread_attr_init(&pthread_custom_attr);

  for(i=0; i<nThreads; i++) {
    tids[i]=i;
    //CREATE_WITH_ARG(bs_thread, &tids[i]);
    pthread_create(&threads[i], &pthread_custom_attr, (void *(*)(void *)) bs_thread, &tids[i]);
  }
  for (i = 0; i < nThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  //  WAIT_FOR_END(nThreads);
#else//ENABLE_THREADS

#ifdef ENABLE_OPENMP
  {
    int tid=0;
    omp_set_num_threads(nThreads);
    bs_thread(&tid);
  }
#else //ENABLE_OPENMP

#ifdef ENABLE_CILK
  {
    int tid=0;
    __cilkrts_set_param ((char*)"nworkers", argv[1]);
    bs_thread(&tid);
  }
#endif //ENABLE_CILK

#ifdef ENABLE_TBB
  tbb::task_scheduler_init init(nThreads);

  int tid=0;
  bs_thread(&tid);
#else //ENABLE_TBB
  //serial version
  int tid=0;
  bs_thread(&tid);
#endif //ENABLE_TBB
#endif //ENABLE_OPENMP
#endif //ENABLE_THREADS

  nextTime("Time for Black Scholes");

  //Write prices to output file
  file = fopen(outputFile, "w");
  if(file == NULL) {
    printf("ERROR: Unable to open file `%s'.\n", outputFile);
    exit(1);
  }
  rv = fprintf(file, "%i\n", numOptions);
  if(rv < 0) {
    printf("ERROR: Unable to write to file `%s'.\n", outputFile);
    fclose(file);
    exit(1);
  }
  for(i=0; i<numOptions; i++) {
    rv = fprintf(file, "%.18f\n", prices[i]);
    if(rv < 0) {
      printf("ERROR: Unable to write to file `%s'.\n", outputFile);
      fclose(file);
      exit(1);
    }
  }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file `%s'.\n", outputFile);
    exit(1);
  }

  free(data);
  free(prices);

  return 0;
}
