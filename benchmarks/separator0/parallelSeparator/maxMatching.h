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
#include "gettime.h"
#include "parallel.h"
#include "graph.h"
using namespace std;

// **************************************************************
//    MAXIMAL MATCHING ON A TREE
// **************************************************************

// Holds state per vertex
struct vertexD {
  uintT ngh; // the neighbor in the matching, initially INT_MAX
  uintT hook; // temporary pointer to hooked vertex
  bool flag; // 1 if matched or all neighbors are matched (all 1 at end)
};


// Returns 1 for vertices that are not yet matched.
struct notYetMatchedT_FF {
  vertexD *GD;
  notYetMatchedT_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (intT v) {
    uintT hook = GD[v].hook;
    if (hook == UINT_T_MAX) return 0;
    if (GD[hook].ngh == v && GD[hook].flag == 0) GD[hook].ngh = UINT_T_MAX;
    if (GD[v].flag == 1) return 0;
    GD[v].ngh = UINT_T_MAX;
    return 1;
  }
};

void maxMatchTR(uintT* RR, uintT size, vertexD* GD, uintT round, uintT maxRound) {
  //cout << "MatchingT: size = " << size << " round = " << round << endl;

  if (size > 0) {
    // avoiding infinite loops
    utils::myAssert(round <= maxRound,"maximalMatching: too many rounds"); 

    uintT ssize = size;
    if (ssize > 10000) ssize = ssize/4;

    parallel_for (uintT i=0; i < ssize; i++) {
      uintT v = RR[i];
      uintT hook = GD[v].hook;
      if (hook != UINT_T_MAX) {
	if (GD[v].flag == 0 && GD[hook].flag == 0) {
	  utils::writeMin(&GD[v].ngh,v);
	  utils::writeMin(&GD[hook].ngh,v);
	}
	else GD[v].hook = UINT_T_MAX;
      }
    }


    parallel_for (uintT ii=0; ii < ssize; ii++) {
      uintT v = RR[ii];
      uintT hook = GD[v].hook;
      if (GD[v].hook != UINT_T_MAX && GD[hook].ngh == v && GD[v].ngh == v) {
	GD[v].ngh = hook;
	GD[v].flag = GD[hook].flag = 1;
      } 
    }

    // Since many heads might write to the same tail, this filter checks if
    // you agree that you are each others neighbors and keeps any unmatched
    // vertices.

    uintT* R = newA(uintT,ssize);
    uintT l = sequence::filter(RR,R,ssize,notYetMatchedT_FF(GD));
    uintT o = ssize-l;
    parallel_for (uintT iii=0; iii < l; iii++) RR[o+iii] = R[iii];
    delete R;

    // recurse on unmatched vertices
    maxMatchTR(RR+o, size-o, GD, round+1, maxRound);
  } 
}

// Finds a maximal matching of the graph
// Returns an array with one element per vertex which is either set
// to the matching vertex id, or to -2 if not match.
// For some reason putting a parallel_for in this code breaks
uintT* maximalMatchingT(uintT* T,uintT n) {
  //intT n = T.size();
  uintT round = 0;
  uintT* R = newA(uintT,n);
  parallel_for(uintT i=0;i<n;i++) R[i] = i;
  //seq<intT> R = seq<intT>(n,utils::identityF<intT>());
  vertexD* GD = newA(vertexD,n);
  parallel_for(uintT i=0;i<n;i++) {
    GD[i].ngh = UINT_T_MAX;
    GD[i].hook = T[i];
    GD[i].flag = (T[i] == UINT_T_MAX) ? 1 : 0;
  }

  //seq<vertexD> GD = seq<vertexD>(n,initTGD(T.S));
  // if it runs form more than 100 rounds, something is wrong
  maxMatchTR(R,n,GD,round,round + 500); 
  free(R);
  uintT* Ngh = newA(uintT,n);
  parallel_for (uintT j=0; j<n; j++) Ngh[j] = GD[j].ngh;
  
  //seq<intT> Ngh = GD.map<intT>(copyGD());
  free(GD);
  return Ngh;  
}  
