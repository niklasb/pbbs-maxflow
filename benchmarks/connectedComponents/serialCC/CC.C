// This code is part of the paper "A Simple and Practical Linear-Work
// Parallel Algorithm for Connectivity" in Proceedings of the ACM
// Symposium on Parallelism in Algorithms and Architectures (SPAA),
// 2014.  Copyright (c) 2014 Julian Shun, Laxman Dhulipala and Guy
// Blelloch
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

#include "graph.h"
#include "utils.h"
#include "parallel.h"
using namespace std;

void BFS(graph<intT> GA, intT start, intT* labels, intT* Frontier) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  labels[start] = start;
  Frontier[0] = start;
  intT bot = 0;
  intT top = 1;

  while (top > bot) {
    intT v = Frontier[bot++];
    for (intT j=0; j < G[v].degree; j++) {
      intT ngh = G[v].Neighbors[j];
      if (labels[ngh] == INT_T_MAX) {
	Frontier[top++] = ngh;
	labels[ngh] = start; //give it a component ID equal to start vertex
      }
    }
  }
}

intT* CC(graph<intT>& GA, float beta) {
  intT n = GA.n;
  intT* labels = newA(intT,n);
  for(intT i=0;i<n;i++) labels[i] = INT_T_MAX;

  intT* Frontier = newA(intT,n);

  for(intT i=0;i<n;i++) {
    if(labels[i] == INT_T_MAX) BFS(GA,i,labels,Frontier);
  }

  free(Frontier);
  return labels;
}
