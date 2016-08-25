#include <iostream>
#include "limits.h"
#include "seq.h"
#include "gettime.h"
#include "cilk.h"
#include "graph.h"
using namespace std;

// **************************************************************
//    MAXIMAL MATCHING
// **************************************************************

// Holds state per vertex
struct vertexD {
  int ngh; // the neighbor in the matching, initially INT_MAX
  int hook; // temporary pointer to hooked vertex
  bool flag; // 1 if matched or all neighbors are matched (all 1 at end)
};

inline int hashVertex(int round, vindex i) {
  return int ((((unsigned int) -1) >> 1) & utils::hash(round + i));
}

// Returns 1 for vertices that are not yet matched.
struct notYetMatchedT_FF {
  vertexD *GD;
  notYetMatchedT_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (vindex v) {
    vindex hook = GD[v].hook;
    if (hook < 0) return 0;
    if (GD[hook].ngh == v && GD[hook].flag == 0) GD[hook].ngh = INT_MAX;
    if (GD[v].flag == 1) return 0;
    GD[v].ngh = INT_MAX;
    return 1;
  }
};

void maxMatchTR(vindex* RR, int size, vertexD* GD, int round, int maxRound) {
  //cout << "MatchingT: size = " << size << " round = " << round << endl;

  if (size > 0) {
    // avoiding infinite loops
    utils::myAssert(round <= maxRound,"maximalMatching: too many rounds"); 

    int ssize = size;
    if (ssize > 10000) ssize = ssize/4;

    cilk_for (int i=0; i < ssize; i++) {
      vindex v = RR[i];
      vindex hook = GD[v].hook;
      if (hook >= 0) {
	if (GD[v].flag == 0 && GD[hook].flag == 0) {
	  utils::writeMin(&GD[v].ngh,v);
	  utils::writeMin(&GD[hook].ngh,v);
	}
	else GD[v].hook = -1;
      }
    }

//     for (int i=0; i < size; i++) {
//       vindex v = RR[i];
//       cout << "v=" << v << " hook = " << (GD[v].hook) << " ngh=" << GD[v].ngh << " flag=" << GD[v].flag << endl;
//     }

    cilk_for (int ii=0; ii < ssize; ii++) {
      vindex v = RR[ii];
      vindex hook = GD[v].hook;
      if (GD[v].hook >= 0 && GD[hook].ngh == v && GD[v].ngh == v) {
	GD[v].ngh = hook;
	GD[v].flag = GD[hook].flag = 1;
      } 
    }

    // Since many heads might write to the same tail, this filter checks if
    // you agree that you are each others neighbors and keeps any unmatched
    // vertices.

    vindex* R = newA(vindex,ssize);
    int l = sequence::filter(RR,R,ssize,notYetMatchedT_FF(GD));
    int o = ssize-l;
    cilk_for (int iii=0; iii < l; iii++) RR[o+iii] = R[iii];
    delete R;

    // recurse on unmatched vertices
    maxMatchTR(RR+o, size-o, GD, round+1, maxRound);
  } 
}

// Only needed because of a bug in Cilk++
struct initTGD { 
  vindex* T;
  initTGD(vindex* TT) : T(TT) {}
  vertexD operator() (int i) {
    vertexD VD; 
    VD.ngh= INT_MAX; 
    VD.hook = T[i]; 
    VD.flag = (T[i] == -1) ? 1 : 0; // if T[i]==-1, then no hook
    return VD;}
};

struct copyGD { vindex operator() (vertexD v) {return v.ngh;}};

// Finds a maximal matching of the graph
// Returns an array with one element per vertex which is either set
// to the matching vertex id, or to -2 if not match.
// For some reason putting a cilk_for in this code breaks
vindex* maximalMatchingT(seq<vindex> T) {
  int n = T.size();
  int round = 0;
  seq<vindex> R = seq<vindex>(n,utils::identityF<int>());
  seq<vertexD> GD = seq<vertexD>(n,initTGD(T.S));
  // if it runs form more than 100 rounds, something is wrong
  maxMatchTR(R.S,R.size(),GD.S,round,round + 500); 
  R.del();
  //vindex* Ngh = newA(vindex,n);
  //cilk_for (int j=0; j<n; j++) Ngh[j] = GD.S[j].ngh;
  seq<vindex> Ngh = GD.map<vindex>(copyGD());
  GD.del();
  return Ngh.S;  
}  

void checkMaximalMatchingT(seq<vindex> T, vindex* GD) {
  int n = T.size();
  for (int i=0; i < n; i++) {
    if (GD[i] == INT_MAX) {
      if (GD[T[i]] == INT_MAX) {
	cout << "unassigned vertex: " << i << "," << T[i] << endl;
	abort();
      }
    } else {
      if (GD[GD[i]] != i) {
	cout << "misassigned vertex: " << i << "," << GD[i]
	     << "," << GD[GD[i]] << endl;
	abort();
      }
      //else cout << i << "," << GD[i] << endl;
    }
  }
}

// Returns 1 for vertices that are not yet matched.
struct notYetMatched_FF {
  vertexD *GD;
  notYetMatched_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (vindex v) {
    vindex hook = GD[v].hook;
    if (hook >= 0) GD[hook].ngh = INT_MAX;
    return !GD[v].flag;
  }
};

void maxMatchROld(seq<vindex> Remain, vertex* G, vertexD* GD, int round, int maxRound) {
  //cout << "size = " << Remain.size() << " round = " << round << endl;
  int size = Remain.size();
  vindex* RR = Remain.S;

  if (Remain.size() > 0) {  
    // avoiding infinite loops
    utils::myAssert(round <= maxRound,"maximalMatching: too many rounds"); 

    int ssize = size;
    if (ssize > 10000) ssize = ssize/10;

    cilk_for (int i=0; i < ssize; i++) {
      vindex v = RR[i];
      if (GD[v].flag == 0) {
	int fl=0;
	GD[v].hook = -1;
	int jj = hashVertex(0,v)%G[v].degree;
	for (int j=0; j < G[v].degree; j++, jj++) {
	  if (jj == G[v].degree) jj = 0;
	  vindex ngh = G[v].Neighbors[jj];
	  //cout << "d= " << G[v].degree << " v= " << v << " ngh= " << ngh << endl;
	  if (GD[ngh].flag == 1) fl++;
	  else if (ngh > v) {
	    utils::writeMin(&GD[ngh].ngh,v);
	    GD[v].hook = ngh;
	    break;}
	}
	// all neighbors are matched -- can't match
	if (fl == G[v].degree) GD[v].flag = 1;
      }
    }

//        for (int i=0; i < Remain.size(); i++) {
//          vindex v = Remain[i];
//          cout << "v=" << v << " hook = " << (GD[v].hook) << " ngh=" << GD[v].ngh << " flag=" << GD[v].flag << endl;
//        }

    cilk_for (int ii=0; ii < ssize; ii++) {
      vindex v = RR[ii];
      if (GD[v].hook >= 0) {
	vindex hook = GD[v].hook;
	if (GD[hook].ngh == v) {
	  if (GD[v].ngh == INT_MAX) {
	    GD[v].flag = GD[hook].flag = 1;
	    GD[v].ngh = hook;
	    GD[v].hook = -1;
	  } 
	} else GD[v].hook = -1;
      }
    }

    // Since many heads might write to the same tail, this filter checks if
    // you agree that you are each others neighbors and keeps any unmatched
    // vertices.

    vindex* R = newA(vindex,ssize);
    int l = sequence::filter(RR,R,ssize,notYetMatched_FF(GD));
    int o = ssize-l;
    cilk_for (int iii=0; iii < l; iii++) RR[o+iii] = R[iii];
    delete R;

    // recurse on unmatched vertices
    maxMatchROld(seq<vindex>(RR+o,size-o), G, GD, round+1, maxRound);
  } 
}

// Returns 1 for vertices that are not yet matched.
struct notYetMatched2_FF {
  vertexD *GD;
  notYetMatched2_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (vindex v) { 
    if (GD[v].flag || GD[v].hook == -2) return 0;
    else return 1;}
};

// Returns 1 for vertices that are not yet matched.
struct noChance_FF {
  vertexD *GD;
  noChance_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (vindex v) { return (GD[v].hook != -1);}
};

void maxMatchR(seq<vindex> Remain, vertex* G, vertexD* GD, int round, int maxRound) {
  //cout << "Matching: size = " << Remain.size() << " round = " << round << endl;

  if (Remain.size() > 0) {  
    // avoiding infinite loops
    utils::myAssert(round <= maxRound,"maximalMatching: too many rounds"); 


    // hook a vertex if possible
    //startTime();
    if (round == 0) {
      cilk_for (int i=0; i < Remain.size(); i++) {
        vindex v = Remain.S[i];
	GD[v].hook = G[v].Neighbors[G[v].degree/2];
      }
    } else {
      cilk_for (int i=0; i < Remain.size(); i++) {
	vindex v = Remain.S[i];
	GD[v].hook = -2;
	for (int j=0; j < G[v].degree; j++) {
	  vindex ngh = G[v].Neighbors[j];
	  if (ngh > v && GD[ngh].flag != 1) {
	    GD[v].hook = ngh;
	    break;}
	}
      }
    }
    //nextTime("Pick edge");

    //startTime();
    seq<vindex> RR = Remain.copy();
    maxMatchTR(RR.S, RR.size(), GD, 0, 500);
    RR.del();

    seq<vindex> R = Remain.filter(notYetMatched2_FF(GD));
    //nextTime("MaxMatchTR");

    // recurse on unmatched vertices
    maxMatchR(R, G, GD, round+1, maxRound);
    R.del();
  } 
}

// Only needed because of a bug in Cilk++
struct initGD { 
  vertexD operator() (int i) {vertexD VD; VD.ngh= INT_MAX; VD.hook = -1; VD.flag=0; return VD;}
};

// Finds a maximal matching of the graph
// Returns an array with one element per vertex which is either set
// to the matching vertex id, or to -2 if not match.
// For some reason putting a cilk_for in this code breaks
vindex* maximalMatching(graph G, int seed) {
  int n = G.n;
  int round = seed;
  seq<vindex> R = seq<vindex>(n,utils::identityF<int>());
  seq<vertexD> GD = seq<vertexD>(n,initGD());
  // if it runs form more than 100 rounds, something is wrong
  maxMatchR(R,G.V,GD.S,round,round + 500); 
  R.del();
  //vindex* Ngh = newA(vindex,n);
  //cilk_for (int j=0; j<n; j++) Ngh[j] = GD.S[j].ngh;
  seq<vindex> Ngh = GD.map<vindex>(copyGD());
  GD.del();
  return Ngh.S;  
}  

// Checks for every vertex if locally maximally matched
void checkMaximalMatching(graph GG, vindex* GD) {
  int n = GG.n;
  vertex* G = GG.V;
  cilk_for (int i=0; i < n; i++) {
    if (GD[i] == INT_MAX) {
      for (int j=0; j < G[i].degree; j++) {
	vindex ngh = G[i].Neighbors[j];
	if (GD[ngh] == INT_MAX) {
	  cout << "unassigned vertex: " << i << "," << ngh << endl;
	  abort();
	} 
      } 
    } else {
      if (GD[GD[i]] != i) {
	cout << "misassigned vertex: " << i << "," << GD[i]
	     << "," << GD[GD[i]] << endl;
	abort();
      }
      //else cout << i << "," << GD[i] << endl;
    }
  }
}

