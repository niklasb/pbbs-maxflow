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

struct BFS_thresholdF {
  intT threshold;
  BFS_thresholdF(intT _threshold) : threshold(_threshold) {}
  bool operator() (intT val) {
    return (val < threshold);
  }
};

struct BFS_Add {
  intT operator() (intT& a, intT& b){
    if(a == -1) 
      return b;
    else return a;
  }
  void addAtomic (intT& a, intT& b) {
    if(a == -1) 
      a=b;
  }
};

struct BFS_Mult {
  void operator() (intT& a, intT& b) {
  }
};

bool BFS_isZero(intT a) { return a == -1;}

typedef pair<intT,intT> intPair;

timer t1,t2;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  matrix<intT> M(GA,-1);

  intT n = GA.n;
  pair<intT,intT>* initFrontier = newA(intPair,GA.n);
  intT initFrontierSize = 1;
  initFrontier[0] = make_pair(start,start);

  vec<intT> Frontier(n,initFrontier,0,BFS_isZero,-1,initFrontierSize);
  
  intT* initParents = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) initParents[i] = -1;
  initParents[start] = start;

  vec<intT> Parents(n,initParents,1,BFS_isZero,-1);
  
  intT* Visited = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++)Visited[i]=0;
  Visited[start] = 1;
  
  // Frontier.printSparse();
  // Frontier.toDense();
  // Frontier.printDense();
  intT round = 0;
  while(1){
    round++;
    //cout<<"Round "<<round<<" "<<M.lastFrontierSize<<" "<<M.lastFrontierEdgeCount;
    t1.start();
    vec<intT> output = SparseMVmultOR(M,Frontier,BFS_Add(),BFS_Mult(),BFS_thresholdF((GA.n+GA.m)/10),Visited);
    t1.stop();
    
    if(output.m == 0) break;

    t2.start();
    Parents.add(output,BFS_Add());
    t2.stop();
    //Parents.printDense();
    Frontier.del();
    Frontier = output;
  }  
  t6.reportTotal("Compute frontier edges");
  t3.reportTotal("Matrix");
  t4.reportTotal("Filter");
  t5.reportTotal("Copy and clear");
  t1.reportTotal("Matrix multiply");
  t2.reportTotal("Merging");

  free(Visited);
  Frontier.del();
  Parents.del();
  M.del();

  return make_pair(0,0);
}
