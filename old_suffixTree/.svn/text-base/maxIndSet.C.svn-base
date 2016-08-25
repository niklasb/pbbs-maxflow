#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "graph.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

// **************************************************************
//    MAXIMAL INDEPENDENT SET
// **************************************************************

struct stillSearching_FF {
  vertex *_G;
  int *_Flags;
  stillSearching_FF(vertex *G, int *Flags) : _G(G), _Flags(Flags) {}
  int operator() (vindex v) {
    if (_Flags[v] == 1) return 0;
    // if any neighbor is selected then drop out and set flag=2
    for (int i=0; i < _G[v].degree; i++) {
      vindex ngh = _G[v].Neighbors[i];
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
void maxIndSetR(seq<vindex> Remain, vertex* G, int* Flags, 
		int round, int maxRound) {
  //cout << "size = " << Remain.size() << " round = " << round << endl;
  // avoiding infinite loops
  utils::myAssert(round <= maxRound,"maximalIndependentSet: too many rounds"); 
  if (Remain.size() > 0) {
    // Checks if each vertex is the local maximum based on hash of vertex id
    cilk_for (int i=0; i < Remain.size(); i++) {
      vindex v = Remain[i];
      unsigned int h = utils::hash(round + v);
      Flags[v] = 1;
      for (int j=0; j < G[v].degree; j++) {
	vindex ngh = G[v].Neighbors[j];
	// there is a harmless race condition here since ngh->flag
	// could be 0 or 1 depending on order
	if (Flags[ngh] < 2 && utils::hash(round + ngh) >= h) { 
	  Flags[v] = 0; break; }
      }
    }
    seq<vindex> R = Remain.filter(stillSearching_FF(G,Flags));
    maxIndSetR(R, G, Flags,round+1, maxRound);
    R.del();
  } 
}

// Checks if valid maximal independent set
void checkMaximalIndependentSet(graph G, int* Flags) {
  int n = G.n;
  vertex* V = G.V;
  cilk_for (int i=0; i < n; i++) {
    int nflag;
    for (int j=0; j < V[i].degree; j++) {
      vindex ngh = V[i].Neighbors[j];
      if (Flags[ngh] == 1)
	if (Flags[i] == 1) {
	  cout << "checkMaximalIndependentSet: bad edge " 
	       << i << "," << ngh << endl;
	  abort();
	} else nflag = 1;
    }
    if ((Flags[i] != 1) && (nflag != 1)) {
      cout << "checkMaximalIndependentSet: bad vertex " << i << endl;
      abort();
    }
  }
}

int* maxIndependentSet(graph G, int seed) {
  int n = G.n;
  int round = seed;
  seq<vindex> Remain = seq<vindex>(n, utils::identityF<int>());
  seq<int> Flags = seq<int>(n, utils::zeroF<int>());
  maxIndSetR(Remain,G.V,Flags.S,round,round+100);
  //checkMaximalIndependentSet(G,Flags.S);
  Remain.del();
  return Flags.S;
}
