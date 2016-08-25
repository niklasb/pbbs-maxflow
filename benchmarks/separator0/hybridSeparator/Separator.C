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

#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
using namespace std;

void hybrid(graph<uintT> G, uint* R){
  long k = 0;
  long n = G.n;
  uint* stack = newA(uint,n);
  bool* flags = newA(bool,n);
  parallel_for(long i=0;i<n;i++) flags[i] = 0;
  for(long i=0;i<n;i++) {
    if(!flags[i]){
      long ptr = 0;
      stack[0] = i;
      R[i] = k++;
      while (ptr >= 0) {
	uint vertex = stack[ptr--];
	flags[vertex] = 1;
	if(G.V[vertex].degree > 0) {
	  for(long i = G.V[vertex].degree-1;i>=0;i--){
	    uint ngh = G.V[vertex].Neighbors[i];
	    if(!flags[ngh]) {
	      flags[ngh] = 1;
	      stack[++ptr] = ngh;
	      R[ngh] = k++;
	    }
	  }
	}
      }
    }
  }
  free(stack);
  free(flags);
}

uint* separator(graph<uintT> G) {
  long n = G.n;
  uint* R = newA(uint,n);
  hybrid(G,R);
  return R;
}

