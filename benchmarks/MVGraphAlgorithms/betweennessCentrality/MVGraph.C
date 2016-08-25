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
#include <vector>
using namespace std;

struct BC_thresholdF {
  intT threshold;
  BC_thresholdF(intT _threshold) : threshold(_threshold) {}
  bool operator() (intT val) {
    return (val < threshold);
  }
};

struct BC_Add {
  double operator() (double a, double b){
    return a+b;
  }
  void addAtomic (double& a, double b) {
    utils::writeAdd(&a,b);
  }
};

struct BC_Mult {
  double operator() (double a, double b) {
    return a;
  }
};

bool BC_isZero(double a) { return a == 0.0;}

typedef pair<double,intT> doubleIntPair;

timer t1,t2;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  matrix<double> M(GA,0.0);

  intT n = GA.n;
  pair<double,intT>* initFrontier = newA(doubleIntPair,GA.n);
  intT initFrontierSize = 1;
  initFrontier[0] = make_pair(1,start);

  vec<double> Frontier(n,initFrontier,0,BC_isZero,0.0,initFrontierSize);
  
  double* initNumPaths = newA(double,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) initNumPaths[i] = 0;
  initNumPaths[start] = 1;

  vec<double> NumPaths(n,initNumPaths,1,BC_isZero,0.0);
  
  intT* Visited = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++)Visited[i]=0;
  Visited[start] = 1;

  intT round = 0;
  
  vector<vec<double> > Levels;
  Frontier.printSparse();
  while(1){
    round++;
    cout<<"Round "<<round<<" "<<M.lastFrontierSize<<" "<<M.lastFrontierEdgeCount;
    //t1.start();
    vec<double> output = MVmult(M,Frontier,BC_Add(),BC_Mult(),BC_thresholdF((GA.n+GA.m)/10),Visited);
    //t1.stop();
 
    Levels.push_back(Frontier);
    if(output.m == 0) {Levels.push_back(output); break;}

    //t2.start();
    NumPaths.add(output,BC_Add());
    //t2.stop();

    Frontier = output;
  }
  
  // t1.reportTotal("matrix multiply");
  // t2.reportTotal("merging");

  free(Visited);
  NumPaths.del();
  M.del();
  parallel_for(intT i=0;i<=round;i++){
    Levels[i].del();
  }

  return make_pair(0,0);
}
