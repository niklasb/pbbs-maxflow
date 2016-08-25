// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
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

#include "parallel.h"
#include "utils.h"
#include "randPerm.h"
#include <iostream>
#include "sequence.h"
#include "speculative_for.h"
#include "listRanking.h"
using namespace std;

struct listRankingStep {
  intT* R;
  node* nodes;
  listRankingStep(intT* _R, node* _nodes) : R(_R), nodes(_nodes) {}

  bool reserve(intT i) {
    if(nodes[i].next == -1 || nodes[i].prev == -1) return 0;
    reserveLoc(R[i], i);
    reserveLoc(R[nodes[i].next], i);
    return 1;
  }

  bool commit (intT i) {
    if(R[i] == i) {
      if(R[nodes[i].next] == i) {
	intT next = nodes[i].next;
	intT prev = nodes[i].prev;
	nodes[next].prev = prev;
	nodes[prev].next = next;
	R[next] = INT_T_MAX;
	return 1;
      } else R[i] = INT_T_MAX;
    } else if(R[nodes[i].next] == i) R[nodes[i].next] = INT_T_MAX;
    return 0;
  }
};

void listRanking(node *A, intT n, intT r = -1) {
  intT* R = newArray(n, (intT) INT_T_MAX);
  listRankingStep lStep(R,A);
  speculative_for(lStep, 0, n, (r != -1) ? r : 100, 0);
  free(R);
}
