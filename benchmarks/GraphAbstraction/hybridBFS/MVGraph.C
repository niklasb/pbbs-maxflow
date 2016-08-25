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

void levels(intT start, intT* L, graph<intT> G){
  intT* Q = newA(intT,G.n);
  intT start_ptr = 0, end_ptr = 0;
  Q[end_ptr++] = start;
  intT level = 0;
  L[start] = level;
  intT next_end_ptr = 1;
  while(1){
    level++;
    for(intT i=start_ptr;i<end_ptr;i++){
      intT v = Q[i];
      for(intT j=0;j<G.V[v].degree;j++){
	intT ngh = G.V[v].Neighbors[j];
	if(L[ngh] == -1){
	  L[ngh] = level;
	  Q[next_end_ptr++] = ngh;
	}
      }
    }
    if(end_ptr == next_end_ptr) break;
    start_ptr = end_ptr; 
    end_ptr = next_end_ptr;
  }
  free(Q);
}


// Checks if T is valid BFS tree relative to G starting at i
void checkBFS(intT start, graph<intT> G, intT* Parents) {

  intT* L = newA(intT, G.n);
  parallel_for (intT i=0; i < G.n; i++) {
    L[i] = -1;
  }
  levels(start,L,G);

  if(Parents[start] != start) {
    cout<<"Parent of root not itself\n";
    abort();
  }
  parallel_for(intT i=0;i<G.n;i++) {
    if(i != start && L[i] != -1){
      if(L[Parents[i]] != L[i]-1){
	cout<<"Level of "<<i<<" wrong\n";
	abort();
      }
    }
  }
  free(L);
 
}


struct BFS_Add {
  intT* Parents;
  BFS_Add(intT* _Parents) : Parents(_Parents) {}
  inline bool add (intT s, intT d) {
    if(Parents[d] == -1) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool addAtomic (intT s, intT d){
    return (utils::CAS(&Parents[d],(intT)-1,s));
  }
  inline bool cond (intT d) { return (Parents[d] == -1); }
};

timer t1,t2;


pair<intT,intT> MVGraph(intT start, graph<intT> GA) {

  startTime();
  intT n = GA.n;
  bool denseForward = 0;
  intT* Parents = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) Parents[i] = -1;
  Parents[start] = start;

  vertices Frontier(n,start);

  intT round = 0;
  timer t1;

  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t1.start();
    vertices output = edgeMap(GA, Frontier, BFS_Add(Parents),GA.m/20,denseForward);
    t1.stop();
    Frontier.del();
    Frontier = output;
  } 
  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t1.reportTotal("MVMult");
  Frontier.del();
  //nextTime("total hybrid time");
  //checkBFS(start, GA, Parents);
  free(Parents); 
 
  return make_pair(0,0);
}
