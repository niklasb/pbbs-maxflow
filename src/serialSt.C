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
  do j = parent[j]; while (parent[j] >= 0);
  parent[i] = j;
  return j;
}

//serial spanning tree
edgeArray st(edgeArray EA){
  edge* E = EA.E;
  int m = EA.nonZeros;
  vindex *parents = newA(vindex,m);
  for(int i=0;i<m;i++) parents[i] = -1;
  edge* st = newA(edge,1);
  int* sti = newA(int,m);
  int nInSt = 0; 
  for(int i = 0; i < m; i++){
    vindex u = find(E[i].u,parents);
    vindex v = find(E[i].v,parents);
    if(u != v){
      //union by rank -- join shallower
      //tree to deeper tree
      if(parents[v] < parents[u]) swap(u,v);
      parents[u] += parents[v];
      parents[v] = u;
      sti[nInSt++] = i;
    }
  }  
  free(parents); 
  cout<<"num edges in ST = "<<nInSt<<endl;
  return edgeArray(st,m,m,m);
}
