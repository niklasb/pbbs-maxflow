#include <iostream>
#include <algorithm>
#include "gettime.h"
#include "graph.h"
using namespace std;

// **************************************************************
//    SOME TIMING CODE
// **************************************************************

timer sortingTime;
timer unionFindTime;

void reportDetailedTime() {
  sortingTime.reportTotal("  sorting time");
  unionFindTime.reportTotal("  union/find time");
}

// **************************************************************
//    FIND OPERATION FOR UNION FIND
// **************************************************************

// Assumes root is negative
vindex find(vindex i, vindex* parent) {
  if (parent[i] < 0) return i;
  else {
    vindex j = i;
    vindex tmp;

    // Find root
    do j = parent[j]; while (parent[j] >= 0);

    // path compress
    while ((tmp = parent[i]) != j) { parent[i] = j; i = tmp;}

    return j;
  }
}

// **************************************************************
//    SERIAL MST USING KRUSKAL's ALGORITHM
// **************************************************************

struct edgeLess : std::binary_function <wghEdge,wghEdge,bool> {
  bool operator() (wghEdge const& a, wghEdge const& b) const
  { return (a.weight < b.weight); }
};

int unionFindLoop(wghEdge* E, int m, int nInMst,
		  vindex* parent, wghEdge* mst) {
  for (int i = 0; i < m; i++) {
    vindex u = find(E[i].u, parent);
    vindex v = find(E[i].v, parent);

    // union operation 
    if (u != v) {
      if (parent[v] < parent[u]) swap(u,v);
      parent[u] += parent[v]; 
      parent[v] = u;
      mst[nInMst++] = E[i]; 
    }
  }
  return nInMst;
}

wghEdgeArray mst(wghEdgeArray G) { 
  wghEdge *E = G.E;

  //startTime();
  vindex *parent = new vindex[G.n];
  for (int i=0; i < G.n; i++) parent[i] = -1;
  wghEdge *mst = new wghEdge[G.n];
  int l = min(4*G.n/3,G.m);

  //nextTime("init");
  sortingTime.start();
  nth_element(E, E+l, E+G.m, edgeLess());
  //nextTime("nth");
  sort(E, E+l, edgeLess());
  sortingTime.stop();
  //nextTime("sort");

  int nInMst = unionFindLoop(E, l, 0, parent, mst);
  //nextTime("loop1");
  wghEdge *f = E+l;
  for (wghEdge *e = E+l; e < E + G.m; e++) {
    vindex u = find(e->u, parent);
    vindex v = find(e->v, parent);
    if (u != v) *f++ = *e;
  }
  //nextTime("filter");
  //cout << "kept=" << f-(E+l) << endl;
  sort(E+l, f, edgeLess());
  //nextTime("sort 2");
  nInMst = unionFindLoop(E+l, f-(E+l), nInMst, parent, mst);
  //nextTime("loop 2");
  //cout << "n = " << G.n << " edges in mst = " << nInMst << endl;

  free(parent);
  return wghEdgeArray(mst, G.n, nInMst);
}
