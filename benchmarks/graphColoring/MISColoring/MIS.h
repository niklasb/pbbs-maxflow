#include <iostream>
#include <cstdlib>
#include "sequence.h"
#include "graph.h"
#include "gettime.h"
#include "parallel.h"
using namespace std;

// **************************************************************
//    MAXIMAL INDEPENDENT SET
// **************************************************************

struct stillSearching_FF {
  vertex<intT> *_G;
  intT *_Flags;
  stillSearching_FF(vertex<intT> *G, intT *Flags) : _G(G), _Flags(Flags) {}
  intT operator() (intT v) {
    if (_Flags[v] == 1) return 0;
    // if any neighbor is selected then drop out and set flag=2
    for (intT i=0; i < _G[v].degree; i++) {
      intT ngh = _G[v].Neighbors[i];
      if (_Flags[ngh] == 1) {_Flags[v] = 2; break;}
    }
    if (_Flags[v] == 0) return 1;
    else return 0;
  }
};

// Remain : the indices of vertices still being worked on
//   Flags[i] == 0 means still working on (in Remain)
//   Flags[i] == 1 means in the MIS
//   Flags[i] == 2 means at least one neighbor is in the MIS
void maxIndSetR(_seq<intT> Remain, vertex<intT>* G, intT* Flags, 
		intT round, intT maxRound) {
  //cout << "size = " << Remain.size() << " round = " << round << endl;
  // avoiding infinite loops
  //utils::myAssert(round <= maxRound,"maximalIndependentSet: too many rounds"); 
  if (Remain.n > 0) {
    // Checks if each vertex<intT> is the local maximum based on hash of vertex<intT> id
    cilk_for (intT i=0; i < Remain.n; i++) {
      intT v = Remain.A[i];
      uintT h = utils::hash(round + v);
      Flags[v] = 1;
      for (intT j=0; j < G[v].degree; j++) {
	intT ngh = G[v].Neighbors[j];
	// there is a harmless race condition here since ngh->flag
	// could be 0 or 1 depending on order
	if (Flags[ngh] < 2 && utils::hash(round + ngh) >= h) { 
	  Flags[v] = 0; break; }
      }
    }
    _seq<intT> R = sequence::filter(Remain.A, Remain.n,stillSearching_FF(G,Flags));
    maxIndSetR(R, G, Flags,round+1, maxRound);
    R.del();
  } 
}

intT* maxIndependentSet(graph<intT> G, intT seed) {
  intT n = G.n;
  intT round = seed;
  intT* _Remain = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) _Remain[i] = i;
  _seq<intT> Remain(_Remain,n);

  intT* _Flags = newArray(n,(intT)0);
  _seq<intT> Flags(_Flags,n);
  maxIndSetR(Remain,G.V,Flags.A,round,round+100);
  Remain.del();
  return Flags.A;
}
