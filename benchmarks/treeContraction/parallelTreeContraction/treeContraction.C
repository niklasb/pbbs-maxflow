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
#include "treeContraction.h"
using namespace std;

struct contractStep {
  intT* R;
  intT* nodes;
  internalNode* internal;
  intT n;
  contractStep(intT* _R, intT* _nodes, internalNode* _internal, intT _n) 
    : R(_R), nodes(_nodes), internal(_internal), n(_n) {}

  bool reserve(intT i) {
    intT parent = nodes[i];
    intT grandparent = internal[parent].parent;//nodes[parent];
    if(grandparent == -1) return 0;
    reserveLoc(R[parent],i);
    reserveLoc(R[grandparent],i);
    return 1;
  }

  bool commit(intT i) {
    intT parent = nodes[i];
    intT grandparent = internal[parent].parent;//nodes[parent];
    if(grandparent == -1) return 1;
    if(R[parent] == i) {
      if(R[grandparent] == i) { //sibling should shortcut to grandparent
	intT right = internal[parent].rightChild;
	intT left = internal[parent].leftChild;
	intT sibling = (i == right) ? left : right;
	if(sibling < n) nodes[sibling] = grandparent;
	else internal[sibling].parent = grandparent;
	//nodes[sibling] = grandparent;
	if(internal[grandparent].rightChild == parent) 
	  internal[grandparent].rightChild = sibling;
	else internal[grandparent].leftChild = sibling;
	R[grandparent] = INT_T_MAX;
	return 1;
      } else R[parent] = INT_T_MAX;
    } else if(R[grandparent] == i) R[grandparent] = INT_T_MAX;
    return 0;
  }
};

void treeContraction(intT* nodes, internalNode* internal, intT n, intT r) {
  intT* R = newArray(2*n-1, (intT) INT_T_MAX);
  contractStep cStep(R,nodes,internal,n);
  speculative_for(cStep, 0, n, (r != -1) ? r : 50, 0);
  free(R);
}
