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

#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "deterministicHash.h"
#include "gettime.h"
using namespace std;

// **************************************************************
//    DETERMINISTIC BREADTH FIRST SEARCH USING A HASH TABLE
// **************************************************************

timer insertionTime;
timer packTime;
timer degreeCountTime;
timer copyTime;
timer creationTime;

void reportDetailedTime() {
  degreeCountTime.reportTotal("  degree count time");  
  creationTime.reportTotal("  hash table creation time");
  insertionTime.reportTotal("  hash table insertion time");
  packTime.reportTotal("  hash table packing time");
  copyTime.reportTotal("  copy backPtrs time");
}

typedef pair<intT,intT> intPair;

struct hashBFS {
  typedef intPair eType;
  typedef intT kType;
  eType empty() {return pair<intT,intT>(-2,-2);}
  kType getKey(eType v) { return v.first; }
  uintT hash(intT s) { return utils::hash(s);}
  int cmp(intT v, intT b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType s, eType s2) {return s.second < s2.second;}
};

struct getDegree {
  vertex<intT>* G;
  intT* FR;
  getDegree(intT* _FR, vertex<intT>* GG) : FR(_FR), G(GG) {}
  intT operator() (intT i) {return G[FR[i]].degree;}
};

timer bfstime;

pair<intT,intT*> BFS(intT start, graph<intT> GA) {
  bfstime.start();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;
  intT* backPtrs = newA(intT,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) backPtrs[i] = INT_T_MAX;

  intT fSize = 1;
  backPtrs[start] = -start;
  int round = 0;
  intT totalVisited = 0;
  
  _seq<intT> FRseq;
  intT* FR = newA(intT,1);
  FR[0] = start;

  while (fSize > 0) {
    totalVisited += fSize;
    round++;

    degreeCountTime.start();
    intT degreeCount = 
      sequence::reduce<intT,intT,utils::addF<intT> ,getDegree>
      ((intT)0,fSize,utils::addF<intT>(),getDegree(FR,G));
    degreeCountTime.stop();

    //Table<hashBFS,intT> T(min(degreeCount,numVertices),hashBFS());
    creationTime.start();
    bfstime.stop();
    Table<hashInt<intT>,intT> T(min(degreeCount,numVertices),hashInt<intT>(),1.0);
    bfstime.start();
    creationTime.stop();
    insertionTime.start();
    parallel_for(intT i=0;i<fSize;i++){
      intT v = FR[i];
      for(intT j=0; j<G[v].degree;j++){
	intT ngh = G[v].Neighbors[j];
	if(utils::writeMin(&backPtrs[ngh],v)) T.insert(ngh);
	// if(backPtrs[ngh] == -1) {
	//   T.insert(make_pair(ngh,v));
	// }
      }
    }
    insertionTime.stop();

    packTime.start();
    free(FR);
    FRseq = T.entries();
    packTime.stop();
    bfstime.stop();
    T.del();
    bfstime.start();
    copyTime.start();
    fSize = FRseq.n;
    FR = FRseq.A;
    parallel_for(intT i=0;i<fSize;i++){
      intT v = FR[i];
      backPtrs[v] = -backPtrs[v];
    }
    copyTime.stop();
  }

  copyTime.start();
  parallel_for(intT i=0;i<numVertices;i++) {
    if(backPtrs[i] == INT_T_MAX) backPtrs[i] = -1;
    else backPtrs[i] = -backPtrs[i];
  }
  copyTime.stop();

  if(FR != NULL) free(FR);
  //reportDetailedTime();
  bfstime.reportTotal("bfs time");
  return pair<intT,intT*>(totalVisited,backPtrs);
}
