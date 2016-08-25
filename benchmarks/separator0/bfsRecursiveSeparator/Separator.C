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

//returns furthest visited vertex from start
uint bfs(graph<uintT> G, uint n, uint start, uint* indices, bool* flags){
  uint* stack = newA(uint,n);
  uint last = start;
  long bot = 0;
  long top = 1;
  stack[0] = start;
  while (top > bot) {
    uint vertex = stack[bot++];
    last = vertex;
    flags[vertex] = 1;
    if(G.V[vertex].degree > 0) {
      for(long i = G.V[vertex].degree-1;i>=0;i--){
	uint ngh = G.V[vertex].Neighbors[i];
	if(!flags[ngh]) {
	  flags[ngh] = 1;
	  stack[top++] = ngh;
	}
      }
    }
  }
  
  //reset flags
  bot = 0;
  top = 1;
  stack[0] = start;
  while (top > bot) {
    uint vertex = stack[bot++];
    flags[vertex] = 0;
    if(G.V[vertex].degree > 0) {
      for(long i = G.V[vertex].degree-1;i>=0;i--){
  	uint ngh = G.V[vertex].Neighbors[i];
  	if(flags[ngh]) {
  	  flags[ngh] = 0;
  	  stack[top++] = ngh;
  	}
      }
    }
  }
  free(stack);
  return last;
}

void bfsRecursive(graph<uintT> G, uint* indices, long n, long numVertices, long startLabel, uint* labels, bool* flags) {
  if(n == 1) { labels[indices[0]] = startLabel; free(indices); return; }
  else {
    uint* stack = newA(uint,n);
    long numVisited = 0;
 
    //visit half the nodes
    for(long i=0;i<n;i++) {
      if(numVisited >= n/2) break;
      if(!flags[indices[i]]) {
	uint start = bfs(G,n,indices[i],indices,flags); //find furthest vertex
	long ptr = 0;
	long bot = 0;
	long top = 1;
	stack[0] = start;
	flags[start] = 1;
	numVisited++;
	while (top > bot && numVisited < n/2) {
	  uint vertex = stack[bot++];
	  if(G.V[vertex].degree > 0) {
	    for(long i = G.V[vertex].degree-1;i>=0;i--){
	      if(numVisited >= n/2) break;
	      uint ngh = G.V[vertex].Neighbors[i];
	      if(!flags[ngh]) {
		flags[ngh] = 1;
		stack[top++] = ngh;
		numVisited++;
	      }
	    }
	  }
	}
      }
    }
    free(stack);

    //filter edges
    parallel_for(long i=0;i<n;i++) {
      uint vertex = indices[i];
      bool vflag = flags[vertex];
      long k = 0;
      for(long j=0;j<G.V[vertex].degree;j++) {
	uint ngh = G.V[vertex].Neighbors[j];
	if(vflag == flags[ngh]) G.V[vertex].Neighbors[k++] = ngh;
      }
      G.V[vertex].degree = k;
    }

    //split graph
    uint* indices1 = newA(uint,n/2);
    uint* indices2 = newA(uint,n-n/2);

    bool* flags0 = newA(bool,n), *flags1 = newA(bool,n);
    parallel_for(long i=0;i<n;i++) {
      uint vertex = indices[i];
      flags1[i] = flags[vertex];
      flags0[i] = !flags[vertex];
    }

    sequence::pack(indices,indices1,flags1,n);
    sequence::pack(indices,indices2,flags0,n);
    free(flags0); free(flags1);

    //reset flags
    parallel_for(long i=0;i<n;i++) {
      uint vertex = indices[i];
      if(flags[vertex]) flags[vertex] = 0;
    }

    free(indices);
    //recurse
    
    cilk_spawn bfsRecursive(G,indices1,n/2,numVertices,startLabel,labels,flags);
    bfsRecursive(G,indices2,n-n/2,numVertices,startLabel+n/2,labels,flags);
  }
}

uint* separator(graph<uintT> G) {
  long n = G.n;
  uint* I = newA(uint,n);
  parallel_for(long i=0;i<n;i++) I[i] = i;
  uint* labels = newA(uint,n);
  bool* flags = newA(bool,n);
  parallel_for(long i=0;i<n;i++) flags[i] = 0;
  bfsRecursive(G,I,n,n,0,labels,flags);
  free(flags);
  return labels;
}
