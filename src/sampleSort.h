#include <iostream>
#include <algorithm>
#include "cilk.h"
#include "utils.h"
#include "sequence.h"
#include "gettime.h"
#include "math.h"
#include "quickSort.h"
#include "transpose.h"

template<class E, class BinPred>
void mergeSeq (E* sA, E* sB, int* sC, int lA, int lB, BinPred f) {
  if (lA==0 || lB==0) return;
  E *eA = sA+lA;
  E *eB = sB+lB;
  for (int i=0; i <= lB; i++) sC[i] = 0;
  while(1) {
    while (f(*sA, *sB)) {(*sC)++; if (++sA == eA) return;}
    sB++; sC++;
    if (sB == eB) break;
    if (!(f(*(sB-1),*sB))) {
      while (!f(*sB, *sA)) {(*sC)++; if (++sA == eA) return;}
      sB++; sC++;
      if (sB == eB) break;
    }
  } 
  *sC = eA-sA;
}

#define SSORT_THR 100000
#define AVG_SEG_SIZE 2
#define PIVOT_QUOT 2

template<class E, class BinPred>
void sampleSort (E* A, int n, BinPred f) {
  if (n < SSORT_THR) compSort(A, n, f);  
  else {
    int sq = (int)(pow(n,0.5));
    int rowSize = sq*AVG_SEG_SIZE;
    int numR = (int)ceil(((double)n)/((double)rowSize));
    int numSegs = (sq-1)/PIVOT_QUOT;
    int overSample = 4;
    int sampleSetSize = numSegs*overSample;
    E* sampleSet = newA(E,sampleSetSize);
    //cout << "n=" << n << " num_segs=" << numSegs << endl;

    // generate samples with oversampling
    cilk_for (int j=0; j< sampleSetSize; ++j) {
      int o = utils::hash(j)%n;
      sampleSet[j] = A[o]; 
    }

    // sort the samples
    compSort(sampleSet, sampleSetSize, f);

    // subselect samples at even stride
    E* pivots = newA(E,numSegs-1);
    cilk_for (int k=0; k < numSegs-1; ++k) {
      int o = overSample*k;
      pivots[k] = sampleSet[o];
    }
    free(sampleSet);  
    nextTime("samples");

    E *B = newA(E, numR*rowSize);
    int *segSizes = newA(int, numR*numSegs);
    int *offsetA = newA(int, numR*numSegs);
    int *offsetB = newA(int, numR*numSegs);

    // sort each row and merge with samples to get counts
    cilk_for (int r = 0; r < numR; ++r) {
      int offset = r * rowSize;
      int size =  (r < numR - 1) ? rowSize : n - offset;
      sampleSort(A+offset, size, f);
      mergeSeq(A + offset, pivots, segSizes + r*numSegs, size, numSegs-1, f);
    }
    nextTime("sort and merge");

    // transpose from rows to columns
    sequence::scan(segSizes, offsetA, numR*numSegs, plus<int>(),0);
    transpose<int>(segSizes, offsetB).trans(numR, numSegs);
    sequence::scan(offsetB, offsetB, numR*numSegs, plus<int>(),0);
    blockTrans<E>(A, B, offsetA, offsetB, segSizes).trans(numR, numSegs);
    {cilk_for (int i=0; i < n; ++i) A[i] = B[i];}
    nextTime("transpose");

    free(B); free(offsetA); free(segSizes);

    // sort the columns
    {cilk_for (int i=0; i<numSegs; ++i) {
	int offset = offsetB[i*numR];
	if (i == 0) {
	  sampleSort(A, offsetB[numR], f); // first segment
	} else if (i < numSegs-1) { // middle segments
	  // if not all equal in the segment
	  if (f(pivots[i-1],pivots[i])) 
	    sampleSort(A+offset, offsetB[(i+1)*numR] - offset, f);
	} else { // last segment
	  sampleSort(A+offset, n - offset, f);
	}
      }
    }
    nextTime("last sort");
    free(pivots); free(offsetB);
  }
}

#undef compSort
#define compSort(__A, __n, __f) (sampleSort(__A, __n, __f))

