// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and Harsha Vardhan Simhadri and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <algorithm>
#include "parallel.h"
#include "utils.h"
#include "sequence.h"
#include "gettime.h"
#include "math.h"
#include "quickSort.h"
#include "transpose.h"

template <class ET, class F> 
int search(ET* S, int n, ET v, F f) {
  for (int i=0; i < n; i++)
    if (f(v,S[i])) return i;
  return n;
}

template <class ET, class F> 
int binSearch(ET* S, int n, ET v, F f) {
  ET* T = S;
  while (n > 10) {
    int mid = n/2;
    if (f(v,T[mid])) n = mid;
    else {
      n = (n-mid)-1;
      T = T + mid + 1;
    }
  }
  return T-S+search(T,n,v,f);
}

  void genCounts(intT *Index, intT *counts, intT m, intT n) {
    for (intT i = 0; i < m; i++) counts[i] = 0;
    for (intT j = 0; j < n; j++) counts[Index[j]]++;
  }

  template <class E>
  void relocate(E* A, E* B, intT *Index, intT *offsets, intT n) {
    for (long j = 0; j < n; j++) B[offsets[Index[j]]++] = A[j];
  }

#define SSORT_THR 1

template<class E, class BinPred, class intT>
void sampleSort (E* A, intT n, BinPred f) {
  if (n < SSORT_THR) compSort(A, n, f);  
  else {
    intT numBlocks = 32;
    intT bucketSize = 100000;
    intT overSample = 50;

    intT blockSize = (n-1)/numBlocks + 1;
    intT numBuckets = (n-1)/bucketSize + 1;

    intT sampleSetSize = numBuckets*overSample;
    E* sampleSet = newA(E,sampleSetSize);
    //cout << "n=" << n << " num_segs=" << numSegs << endl;

    // generate samples with oversampling
    for (intT j=0; j< sampleSetSize; ++j) {
      intT o = utils::hash(j)%n;
      sampleSet[j] = A[o]; 
    }

    // sort the samples
    quickSortSerial(sampleSet, sampleSetSize, f);

    // subselect samples at even stride
    E* pivots = newA(E,numBuckets-1);
    for (intT k=1; k < numBuckets; ++k) {
      intT o = overSample*k;
      pivots[k-1] = sampleSet[o];
    }
    free(sampleSet);  
    //nextTime("samples");

    //cout << "numBuckets = " << numBuckets << endl;

    intT* bucketId = newA(intT,n);
    parallel_for(intT i = 0; i < n; i++)
      bucketId[i] = binSearch(pivots,numBuckets-1,A[i],f);
    free(pivots);
    //nextTime("binSearch");

    intT* counts = newA(intT, numBlocks*numBuckets);
    parallel_for (intT i2 = 0; i2 < numBlocks; i2++) { 
      intT offset = i2 * blockSize;
      intT size =  (i2 < numBlocks - 1) ? blockSize : n - offset; 
      genCounts(bucketId + offset, counts + i2 * numBuckets, numBuckets, size);
    }
    //nextTime("count");
      
    // generate offsets
    intT sum = 0;
    for (intT i = 0; i < numBuckets; i++)
      for (intT j = 0; j < numBlocks; j++) {
	intT v = counts[j*numBuckets+i];
	counts[j*numBuckets+i] = sum;
	sum += v;
      }

    intT *bucketStarts = newA(intT,numBuckets+1);
    for (intT i = 0; i < numBuckets; i++)
      bucketStarts[i] = counts[i];
    bucketStarts[numBuckets] = n;

    E* B = newA(E, n);
    parallel_for (intT i3 = 0; i3 < numBlocks; i3++) { 
      intT offset = i3 * blockSize;
      intT size =  (i3 < numBlocks - 1) ? blockSize : n - offset; 
      relocate(A + offset, B, bucketId + offset, 
	       counts + i3 * numBuckets,
	       size);
    }
    //nextTime("relocate");

    parallel_for (intT i4 = 0; i4 < n; i4++) A[i4] = B[i4];
    //nextTime("copy");

    //free(offsets);
    free(counts); free(bucketId); free(B);

    parallel_for (intT r = 0; r < numBuckets; r++) {
      intT size = bucketStarts[r+1] - bucketStarts[r];
      //cout << size << endl;
      quickSortSerial(A + bucketStarts[r], size, f);
    }
    //nextTime("recursive sort");
    free(bucketStarts);

    //for (int i = 0; i < n; i ++) cout << A[i] << endl;
  }
}

#undef compSort
#define compSort(__A, __n, __f) (sampleSort(__A, __n, __f))

