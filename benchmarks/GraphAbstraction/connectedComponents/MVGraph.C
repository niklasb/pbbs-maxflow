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
#include "Abstract.h"
#include "gettime.h"
using namespace std;

void CC_Check(graph<intT> GA, intT* IDs){
  intT n = GA.n;
  parallel_for(intT i=0;i<n;i++){
    intT myID = IDs[i];
    for(intT j=0;j<GA.V[i].degree;j++){
      intT ngh = GA.V[i].Neighbors[j];
      if(myID != IDs[ngh]) {
	cout<<"Node "<<i<<" id is "<<myID<<" but neighbor "<<ngh<<" id is "<<IDs[ngh]<<endl;
	exit(0);
      }
    }
  }
}

struct CC_Add {
  intT* IDs;
  intT* prevIDs;
  CC_Add(intT* _IDs, intT* _prevIDs) : 
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool add(intT s, intT d){
    intT origID = IDs[d];
    if(IDs[s] < origID) {
      IDs[d] = min(origID,IDs[s]);
      if(origID == prevIDs[d]) return 1;
    }
    return 0;
  }
  inline bool addAtomic (intT s, intT d) {
    intT origID = IDs[d];
    return (utils::writeMin(&IDs[d],IDs[s]) 
	    && origID == prevIDs[d]);
  }
  inline bool cond (intT d) { return 1; }
};

struct CC_Vertex_F {
  intT* IDs;
  intT* prevIDs;
  CC_Vertex_F(intT* _IDs, intT* _prevIDs) :
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator () (intT i) {
    prevIDs[i] = IDs[i];
    return 1;
  }
};

timer t1,t2,t0;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  startTime();
  bool denseForward = 0;
  intT n = GA.n;

  intT* IDs = newA(intT,n);
  intT* prevIDs = newA(intT,n);

  parallel_for(intT i=0;i<n;i++) IDs[i] = i;

  bool* frontier = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) frontier[i] = 1;

  vertices Frontier(n,n,frontier);
 
  intT round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t2.start();
    vertexMap(Frontier,CC_Vertex_F(IDs,prevIDs));
    t2.stop();
    t1.start();
    vertices output = edgeMap(GA, Frontier, CC_Add(IDs,prevIDs),GA.m/20,denseForward);
    t1.stop();
    Frontier.del();
    Frontier = output;
  }
  Frontier.del();


  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("Copy");
  nextTime("Total");
  //CC_Check(GA,IDs);
  free(IDs); free(prevIDs);
  return make_pair(0,0);
}
