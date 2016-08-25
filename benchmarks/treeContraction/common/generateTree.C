// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and the PBBS team
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
#include "parallel.h"
#include "IO.h"
#include "parseCommandLine.h"
#include "treeContraction.h"

using namespace std;
using namespace benchIO;

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<n> <filename>");
  intT n = atoi(P.getArgument(4));
  char* fileName1 = P.getArgument(3);
  char* fileName2 = P.getArgument(2);
  char* fileName3 = P.getArgument(1);
  char* fileName4 = P.getArgument(0);
  intT* nodes = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) nodes[i] = -1;
  internalNode* internal = newA(internalNode,n-1);
  parallel_for(intT i=0;i<n-1;i++) internal[i].parent = -1;

  intT* IDs = newA(intT,2*n-1);
  parallel_for(intT i=0;i<2*n-1;i++) IDs[i] = i;
  intT* newIDs = newA(intT,2*n-1);

  internalNode* internalO = internal - n;

  intT numIDs = 2*n-1;
  intT numnodes = n;
  intT numProcessed = 0;
  intT maxID = n;
  //generate a tree
  for(intT i=0;i<n-1;i++) {
    //pick two at random
    intT j = 1;
    intT a = utils::hash(i) % numnodes;
    while(IDs[a] == -1) { a = utils::hash(i+3*j) % numnodes; j++; }
    j = 1;
    intT b = utils::hash2(i) % numnodes;
    while(b == a || IDs[b] == -1) { b = utils::hash2(i+j) % numnodes; j++; }
    if(IDs[a] < n) nodes[IDs[a]] = maxID; else internalO[IDs[a]].parent = maxID; 
    if(IDs[b] < n) nodes[IDs[b]] = maxID; else internalO[IDs[b]].parent = maxID; 
    // nodes[IDs[a]] = maxID;
    // nodes[IDs[b]] = maxID;
    internalO[maxID].leftChild = IDs[a];
    internalO[maxID].rightChild = IDs[b];
    IDs[a] = -1;
    IDs[b] = -1;
    numnodes++; maxID++;
    numProcessed +=2;
    if(numnodes/4 < numProcessed) {
      //pack
      numIDs = sequence::filter(IDs, newIDs, numIDs, nonNegF());
      swap(IDs,newIDs);
      numnodes = numnodes - numProcessed;
      numProcessed = 0;
    }
  }
  free(IDs);
  free(newIDs);

  intT* A = newA(intT,n), *parents = newA(intT,n-1), 
    *leftChildren = newA(intT,n-1), *rightChildren = newA(intT,n-1);

  parallel_for(intT i=0;i<n;i++) A[i] = nodes[i];
  parallel_for(intT i=0;i<n-1;i++){
    parents[i] = internal[i].parent;
    leftChildren[i] = internal[i].leftChild;
    rightChildren[i] = internal[i].rightChild;
  }
  free(nodes); free(internal);
 
  writeIntArrayToFile(A, n, fileName1); 
  writeIntArrayToFile(parents, n-1, fileName2); 
  writeIntArrayToFile(leftChildren, n-1, fileName3); 
  writeIntArrayToFile(rightChildren, n-1, fileName4); 
  
  free(A); free(parents); free(leftChildren); free(rightChildren);
}
