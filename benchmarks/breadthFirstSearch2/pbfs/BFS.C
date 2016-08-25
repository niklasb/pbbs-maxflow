// Copyright (c) 2010, Tao B. Schardl
// Integrated into the Problem Based Benchmark Suite by Julian Shun
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

#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
#include <limits.h>
#include "bag.h"
using namespace std;

// **************************************************************
//    PARALLEL BREADTH FIRST SEARCH
// **************************************************************

#define THRESHOLD 256

void
pbfs_walk_Pennant(Pennant<intT> *p,
		  Bag_reducer<intT>* next,
		  intT* backPtrs,
		  const intT nodes[], const intT edges[],
		  intT nNodes, intT nEdges);

static inline void
pbfs_proc_Node(const intT n[],
	       intT fillSize,
	       Bag_reducer<intT> *next,
	       intT* backPtrs,
	       const intT nodes[],
	       const intT edges[], intT nNodes, intT nEdges)
{
  // Process the current element
  Bag<intT>* bnext = &((*next).get_reference());
  for (intT j = 0; j < fillSize; ++j) { 
    // Scan the edges of the current node and add untouched
    // neighbors to the opposite bag
    intT edgeZero = nodes[n[j]];
    intT edgeLast = (n[j] == nNodes-1) ? nEdges : nodes[n[j]+1];
    intT edge;
    for (intT i = edgeZero; i < edgeLast; ++i) {
      edge = edges[i];
      if(backPtrs[edge] == -1) { //unvisited vertex
	(*bnext).insert(edge);
	backPtrs[edge] = n[j];
      }
    }
  }
}

inline void
pbfs_walk_Bag(Bag<intT> *b,
		     Bag_reducer<intT>* next,
	      intT* backPtrs,
	      const intT nodes[],
	      const intT edges[], intT nNodes, intT nEdges)
{
  if (b->getFill() > 0) {
    // Split the bag and recurse
    Pennant<intT> *p = NULL;

    b->split(&p); // Destructive split, decrements b->getFill()
    cilk_spawn pbfs_walk_Bag(b, next, backPtrs, nodes, edges, nNodes, nEdges);
    pbfs_walk_Pennant(p, next, backPtrs, nodes, edges, nNodes, nEdges);

    cilk_sync;

  } else {
    intT fillSize = b->getFillingSize();
    const intT *n = b->getFilling();

    intT extraFill = fillSize % THRESHOLD;
    cilk_spawn pbfs_proc_Node(n+fillSize-extraFill, extraFill,
			      next, backPtrs,
			      nodes, edges, nNodes,nEdges);
    #pragma cilk grainsize = 1
    cilk_for (intT i = 0; i < fillSize - extraFill; i += THRESHOLD) {
      pbfs_proc_Node(n+i, THRESHOLD,
		     next, backPtrs,
		     nodes, edges, nNodes,nEdges);
    }
    cilk_sync;
  }
}

inline void
pbfs_walk_Pennant(Pennant<intT> *p,
			 Bag_reducer<intT>* next,		
		  intT* backPtrs,
		  const intT nodes[],
		  const intT edges[], intT nNodes, intT nEdges)
{
  if (p->getLeft() != NULL)
    cilk_spawn pbfs_walk_Pennant(p->getLeft(), next, backPtrs, nodes, edges, nNodes, nEdges);

  if (p->getRight() != NULL)
    cilk_spawn pbfs_walk_Pennant(p->getRight(), next, backPtrs, nodes, edges, nNodes, nEdges);

  // Process the current element
  
  const intT *n = p->getElements();

  #pragma cilk grainsize=1
  cilk_for (intT i = 0; i < BLK_SIZE; i+=THRESHOLD) {
    // This is fine as long as THRESHOLD divides BLK_SIZE
    pbfs_proc_Node(n+i, THRESHOLD,
		   next, backPtrs,
		   nodes, edges, nNodes, nEdges);
  }
  delete p;
}

intT pbfs(const intT s, intT* backPtrs, const intT nodes[], const intT edges[], intT nNodes, intT nEdges)
{
  Bag_reducer<intT> *queue[2];
  Bag_reducer<intT> b1;
  Bag_reducer<intT> b2;
  queue[0] = &b1;
  queue[1] = &b2;

  bool queuei = 1;

  backPtrs[s] = s;

  // Scan the edges of the initial node and add untouched
  // neighbors to the opposite bag
  cilk_for (intT i = nodes[s]; i < nodes[s+1]; ++i) {
    if (edges[i] != s) {
      (*queue[queuei]).insert(edges[i]);
      backPtrs[edges[i]] = s;
    }
  }

  while (!((*queue[queuei]).isEmpty())) {
    (*queue[!queuei]).clear();
    pbfs_walk_Bag(&((*queue[queuei]).get_reference()), queue[!queuei], backPtrs, nodes, edges, nNodes, nEdges);
    queuei = !queuei;
  }

  return 0;
}

struct nonNegF{
  intT* A;
  nonNegF(intT* _A) : A(_A) {}
  intT operator() (intT i) {return (intT)(A[i]>=0);}
};

pair<intT,intT*> BFS(intT start, graph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  intT* nodes = GA.vertices();
  intT* edges = GA.edges();
  intT* backPtrs = newA(intT,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) backPtrs[i] = -1;

  pbfs(start, backPtrs, nodes, edges, numVertices, numEdges);
  intT numVisited = 
    sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),nonNegF(backPtrs));

  return pair<intT,intT*>(numVisited,backPtrs);
}
