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
#include <iostream>
#include "sequence.h"
#include "speculative_for.h"
#include "listRanking.h"
using namespace std;

struct listRankingStep {
  bool* R;
  node* nodes;
  intT n;
  listRankingStep(bool* _R, node* _nodes, intT _n) : R(_R), nodes(_nodes), n(_n) {}

  bool reserve(intT i) {
    intT next = nodes[i].next, prev = nodes[i].prev;
    if(i < next && i < prev) R[i] = 1; //check if local min
    return 1; }

  bool commit (intT i) {
    if(R[i] == 1){ //local min 
      intT next = nodes[i].next, prev = nodes[i].prev;
      if(next != n) nodes[next].prev = prev;
      if(prev != n) nodes[prev].next = next;
      return 1; } 
    else return 0; //lost 
  }};

void listRanking(node *A, intT n, intT r = -1) {
  bool* R = newArray(n, (bool) 0);
  listRankingStep lStep(R,A,n);
  speculative_for(lStep, 0, n, (r != -1) ? r : 100, 0);
  free(R);
}
