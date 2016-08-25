#include <iostream>
#include <limits.h>
#include "sequence.h"
#include "gettime.h"
#include "graph.h"
#include "cilk.h"
using namespace std;


// Assumes root is negative
inline vindex find(vindex i, vindex* parent) {
  if ((parent[i]) < 0) return i;
  vindex j = parent[i];     
  if (parent[j] < 0) return j;
  do j = parent[j]; 
  while (parent[j] >= 0);
  parent[i] = j;

  /*
  // partial path compress
  vindex tmp;
  int k = 0;
  while ((tmp = parent[i]) != j) { 
    if (k++ == 5) {parent[i] = j; k = 0;}
    i = tmp;}
  */
  return j;
}

void unionFindLoop(edge* E, int m,
		   vindex* parents, vindex* hooks, int *st, int &nInSt) {
  int roundSize = m/50+1;
  vindex *hold = newA(vindex,roundSize);
  vindex *EP = newA(vindex,roundSize);
  bool *flags = newA(bool,roundSize);
  int nDone = 0;  // number of edges that are done
  int keep = 0;   // number of edges that need to be retried in next round
  int round = 0; 
  //int nInSt = 0;

  while (nDone < m) {
    utils::myAssert(round++ < 1000,"unionFindLoop: too many iterations");
    int size = min(roundSize, m-nDone);
    cilk_for (int i=0; i < size-keep; i++) 
      EP[i+keep] = nDone+keep+i;   // new edge pointers to add

    cilk_for (int i =0; i < size; i++) {
      flags[i] = 0;
      int r = EP[i];
      vindex u = find(E[r].u,parents);
      vindex v = find(E[r].v,parents);
      if (u != v) {
        // "reserve" the two endpoints
	utils::writeMin(&hooks[v],r);
	utils::writeMin(&hooks[u],r);
	flags[i] = 1;
      }
    }

    // keep edges that are not self edges
    int n1   = sequence::pack(EP, hold, flags, size);

    //memset(flags,0,n1);
    cilk_for (int i =0; i < n1; i++) {
      flags[i] = 0; 
      int r = hold[i];
      vindex u = find(E[r].u,parents);
      vindex v = find(E[r].v,parents);
      // check if successfully reserved (i.e. min) in either direction
      //   and hook if so
      if (hooks[u] == r) {
	parents[u] = v;
	if (hooks[v] == r) hooks[v] = INT_MAX;
      } else if (hooks[v] == r) 
	parents[v] = u;

      // if not then it needs to be kept for the next round
      else flags[i] = 1;
    }

    // keep edges that failed to hook for next round
    keep = sequence::pack(hold, EP, flags, n1);

    // add hooked edges to mst
    cilk_for (int j = 0; j < n1; j++) flags[j] = !flags[j];
    //int newInSt = sequence::pack(hold, EP+keep, flags, n1);
    //cilk_for (int j = 0; j < newInSt; j++) st[nInSt+j] = E[EP[keep+j]];
    int newInSt = sequence::pack(hold, st+nInSt, flags, n1);

    nDone += size - keep;
    nInSt += newInSt;
  }
  cout<<"nInSt = "<<nInSt<<endl;
  free(hold); free(EP); free(flags);
}


edgeArray st(edgeArray EA){
  edge* E = EA.E;
  int m = EA.nonZeros;
  vindex *parents = newA(vindex,m);
  cilk_for (int i=0; i < m; i++) parents[i] = -1;
  vindex *hooks = newA(vindex,m);
  cilk_for (int i=0; i < m; i++) hooks[i] = INT_MAX;
  edge* st = newA(edge,1);
  int* sti = newA(int,m);
  int nInSt = 0;
  unionFindLoop(E, m, parents, hooks, sti, nInSt);

  free(parents); free(hooks); 

  return edgeArray(st,nInSt+1,nInSt+1,nInSt);
}
