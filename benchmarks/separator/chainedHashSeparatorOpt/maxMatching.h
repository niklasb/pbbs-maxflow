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
  intT ngh; // the neighbor in the matching, initially INT_MAX
  intT hook; // temporary pointer to hooked vertex
  bool flag; // 1 if matched or all neighbors are matched (all 1 at end)
};


// Returns 1 for vertices that are not yet matched.
struct notYetMatchedT_FF {
  vertexD *GD;
  notYetMatchedT_FF(vertexD* GGD) : GD(GGD) {}
  bool operator () (intT v) {
    intT hook = GD[v].hook;
    if (hook < 0) return 0;
    if (GD[hook].ngh == v && GD[hook].flag == 0) GD[hook].ngh = INT_T_MAX;
    if (GD[v].flag == 1) return 0;
    GD[v].ngh = INT_T_MAX;
    return 1;
  }
};

void maxMatchTR(intT* RR, intT size, vertexD* GD, intT round, intT maxRound) {
  //cout << "MatchingT: size = " << size << " round = " << round << endl;

  if (size > 0) {
    // avoiding infinite loops
    utils::myAssert(round <= maxRound,"maximalMatching: too many rounds"); 

    intT ssize = size;
    if (ssize > 10000) ssize = ssize/4;

    parallel_for (intT i=0; i < ssize; i++) {
      intT v = RR[i];
      intT hook = GD[v].hook;
      if (hook >= 0) {
	if (GD[v].flag == 0 && GD[hook].flag == 0) {
	  utils::writeMin(&GD[v].ngh,v);
	  utils::writeMin(&GD[hook].ngh,v);
	}
	else GD[v].hook = -1;
      }
    }


    parallel_for (intT ii=0; ii < ssize; ii++) {
      intT v = RR[ii];
      intT hook = GD[v].hook;
      if (GD[v].hook >= 0 && GD[hook].ngh == v && GD[v].ngh == v) {
	GD[v].ngh = hook;
	GD[v].flag = GD[hook].flag = 1;
      } 
    }

    // Since many heads might write to the same tail, this filter checks if
    // you agree that you are each others neighbors and keeps any unmatched
    // vertices.

    intT* R = newA(intT,ssize);
    intT l = sequence::filter(RR,R,ssize,notYetMatchedT_FF(GD));
    intT o = ssize-l;
    parallel_for (intT iii=0; iii < l; iii++) RR[o+iii] = R[iii];
    delete R;

    // recurse on unmatched vertices
    maxMatchTR(RR+o, size-o, GD, round+1, maxRound);
  } 
}

// Finds a maximal matching of the graph
// Returns an array with one element per vertex which is either set
// to the matching vertex id, or to -2 if not match.
// For some reason putting a parallel_for in this code breaks
intT* maximalMatchingT(intT* T,intT n) {
  //intT n = T.size();
  intT round = 0;
  intT* R = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) R[i] = i;
  //seq<intT> R = seq<intT>(n,utils::identityF<intT>());
  vertexD* GD = newA(vertexD,n);
  parallel_for(intT i=0;i<n;i++) {
    GD[i].ngh = INT_T_MAX;
    GD[i].hook = T[i];
    GD[i].flag = (T[i] == -1) ? 1 : 0;
  }

  //seq<vertexD> GD = seq<vertexD>(n,initTGD(T.S));
  // if it runs form more than 100 rounds, something is wrong
  maxMatchTR(R,n,GD,round,round + 500); 
  free(R);
  intT* Ngh = newA(intT,n);
  parallel_for (intT j=0; j<n; j++) Ngh[j] = GD[j].ngh;
  
  //seq<intT> Ngh = GD.map<intT>(copyGD());
  free(GD);
  return Ngh;  
}  
