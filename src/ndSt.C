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

  return j;
}


edgeArray st(edgeArray EA){
  edge* E = EA.E;
  int m = EA.nonZeros;
  vindex *parents = newA(vindex,m);
  cilk_for (int i=0; i < m; i++) parents[i] = -1;
  vindex *hooks = newA(vindex,m);
  cilk_for (int i=0; i < m; i++) hooks[i] = INT_MAX;
  edge* st = newA(edge,m);

  bool *flags = newA(bool,m);
  cilk_for(int i=0;i<m;i++) flags[i] = 0;

  timespec req;
  req.tv_sec = 0;
  req.tv_nsec = 1;

  //edge joins only if acquired lock on both endpoints
  cilk_for (int i = 0; i < m; i++) {
    int j = 0;
    while(1){ 
      if(j++ > 100) abort();
      vindex u = find(E[i].u,parents);
      vindex v = find(E[i].v,parents);
      if(u == v) break;
      else {
	utils::CAS(&hooks[v], INT_MAX, i);
	utils::CAS(&hooks[u], INT_MAX, i);
	if(hooks[v] == i){
	  if(hooks[u] == i){
	    parents[u] = v; //arbitrary direction
	    flags[i] = 1;
	    hooks[v] = INT_MAX; //reset hook
	    break;
	  } else hooks[v] = INT_MAX; //lost on u, so reset v
	}
	nanosleep(&req, (struct timespec *) NULL);
      }
    }
  }

  int n = sequence::pack(E, st, flags, m);
  free(parents); free(hooks); free(flags);
  cout<<"nInSt = "<<n<<endl;
  return edgeArray(st,n+1,n+1,n);
}
