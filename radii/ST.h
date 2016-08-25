#include <iostream>
#include <limits.h>
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "quickSort.h"
#include "gettime.h"

using namespace std;

void graphReorder(graph& Gr, uint* I) {
  intT n = Gr.n;
  intT m = Gr.m;
  vertex *V = newA(vertex,Gr.n);
  cilk_for (intT i=0; i < Gr.n; i++) V[I[i]] = Gr.V[i];
  cilk_for (intT i=0; i < Gr.n; i++) {
    for (intT j=0; j < V[i].degree; j++) {
      V[i].Neighbors[j] = I[V[i].Neighbors[j]];
    }
  }
  free(Gr.V);
  Gr.V = V;
  
}

// Assumes root is negative
inline int find(int i, int* parent) {
  if ((parent[i]) < 0) return i;
  int j = parent[i];
  if (parent[j] < 0) return j;
  do j = parent[j]; 
  while (parent[j] >= 0);
  int tmp;
  while ((tmp = parent[i]) != j) {
    parent[i] = j;
    i = tmp;
  }
  return j;
}

template <class Edge>
void unionFindLoop(Edge* E, intT m, int* parents, intT* hooks) {
  intT maxRoundSize = m/80+1;
  intT *hold = newA(intT,maxRoundSize);
  intT *EP = newA(intT,maxRoundSize);
  bool *inTree = newA(bool,maxRoundSize);
  bool *failed = newA(bool,maxRoundSize);
  uint *U = newA(uint,maxRoundSize);
  uint *V = newA(uint,maxRoundSize);
  int round = 0;

  intT numberDone = 0;  // number of edges that are done
  intT keep = 0;   // number of edges that need to be retried in next round
  //intT numberInST = 0;

  while (numberDone < m) {
    utils::myAssert(round++ < 1000,"unionFindLoop: too many iterations");
    intT size = min(maxRoundSize, m - numberDone);
    {parallel_for (intT i=0; i < size-keep; i++) { //cout<<i+keep<<" ";
	EP[i+keep] = numberDone+keep+i; }}  // new edge pointers to add

    {parallel_for (intT i =0; i < size; i++) {
      inTree[i] = 0;
      intT r = EP[i];
      U[i] = find(E[r].u,parents);
      V[i] = find(E[r].v,parents);
      if (U[i] > V[i]) swap(U[i],V[i]);
      if (U[i] != V[i]) {
	utils::writeMin(&hooks[V[i]],r);
	inTree[i] = 1;
      }
      }}

    {parallel_for (intT i =0; i < size; i++) {
      failed[i] = 0;
      if (inTree[i])
	if (hooks[V[i]] == EP[i]) parents[V[i]] = U[i];
	else {
	  failed[i] = 1;
	  hold[i] = EP[i];
	  inTree[i] = 0;
	}
      }}

    // keep edges that failed to hook for next round
    keep = sequence::pack(hold, EP, failed, size);

    // add hooked edges to mst
    //intT newInST = sequence::pack(hold, st+numberInST, inTree, size);

    numberDone += size - keep;
    //numberInST += newInST;
  }
  //cout<<"number in ST = "<<numberInST<<endl;
  free(hold); free(EP); free(inTree); free(failed); free(U); free(V);
  //return numberInST;
}

inline void getCompIds(edge* ComponentIds, int* parents, intT n){
  parallel_for(intT i=0;i<n;i++){
    ComponentIds[i].u = i;
    ComponentIds[i].v = find(i,parents);
  }
}

struct lessedgev {bool operator() (edge e1,
				   edge e2) {
  return e1.v < e2.v;} };

struct component {
  uint* compStarts;
  int numComp;
  void del() {free(compStarts);}
  component(uint* _W, int nc) :
  compStarts(_W),numComp(nc) {}
};


component st(graph& A){
  startTime();
  intT m = A.m;
  intT n = A.n;
  uint* zero = A.V[0].Neighbors;
  edge* E = newA(edge,m);
  parallel_for(intT i=0;i<n;i++) {
    uint* Ngh = A.V[i].Neighbors;
    intT offset = Ngh - zero;
    parallel_for(intT j=0;j<A.V[i].degree;j++){
      E[offset+j] = edge(i,Ngh[j]);
    }
  }
  nextTime("converting to edgeArray");
  cout << "n=" << n << " m=" << m << endl;
  int *parents = newA(int,n);

  parallel_for (intT i=0; i < n; i++) parents[i] = -1;
  intT *hooks = newA(intT,n);
  parallel_for (intT i=0; i < n; i++) hooks[i] = INT_T_MAX;
  unionFindLoop(E, m, parents, hooks);
  free(hooks);
  
  nextTime("computing st");
  free(E);
  edge* ComponentIds = newA(edge,n);

  getCompIds(ComponentIds,parents,n);

  free(parents);
  quickSort(ComponentIds,n,lessedgev());
 
  nextTime("getting/sorting by component IDs");

  uint* offsets = newA(uint,n+1);
  offsets[0] = 1; offsets[n] = 0;
  cilk_for(intT i=1;i<n;i++){
    if(ComponentIds[i].v!=ComponentIds[i-1].v)
      offsets[i] = 1;
    else offsets[i] = 0;
  }

  intT numComps = sequence::plusScan(offsets,offsets,n+1);

  cout<<"numComps = "<<numComps<<endl;

  uint* compStarts = newA(uint,numComps+1);
  compStarts[0] = 0;
  compStarts[numComps] = n;

  cilk_for(intT i=1;i<n;i++){
    if(offsets[i+1]!=offsets[i])
      compStarts[offsets[i]]=i;
  }
  free(offsets);
  uint* iperm = newA(uint,n);
  cilk_for(intT i=0;i<n;i++)
    iperm[ComponentIds[i].u] = i;

 
  graphReorder(A,iperm);
  
  nextTime("reordering graph");
  free(iperm);
  free(ComponentIds);
  return component(compStarts,numComps);

}

