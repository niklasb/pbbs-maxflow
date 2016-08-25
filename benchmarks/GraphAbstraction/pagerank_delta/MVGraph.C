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
#include "math.h"
using namespace std;


struct PR_Add {
  vertex<intT>* V;
  double* Delta;
  double* nghSum;
  PR_Add(vertex<intT>* _V, double* _Delta, double* _nghSum) : 
    V(_V), Delta(_Delta), nghSum(_nghSum) {}
  inline bool add(intT s, intT d){
    nghSum[d] += Delta[s]/V[s].degree;
    return 1;
  }
  inline bool addAtomic (intT s, intT d) {
    utils::writeAdd(&nghSum[d],Delta[s]/V[s].degree);
    return 1;
  }
  inline bool cond (intT d) { return 1; }
};

struct PR_Vertex_F {
  double damping;
  double addedConstant;
  double* p;
  double* Delta;
  double* nghSum;
  double one_over_n;
  double epsilon2;
  PR_Vertex_F(double* _p, double* _Delta, double* _nghSum, double _damping, double _one_over_n,double _epsilon2) :
    p(_p),
    damping(_damping), Delta(_Delta), nghSum(_nghSum), 
    addedConstant((1-_damping)*_one_over_n), one_over_n(_one_over_n),
    epsilon2(_epsilon2) {}
  inline bool operator () (intT i) {
    bool start = (p[i] == 0.0) ? 1 : 0;
    if(!start){ //hack because initial delta is messed up as p starts at 0.0
      double old_nghSum = (p[i]-addedConstant)/damping;
      Delta[i] = damping*(old_nghSum+nghSum[i])+addedConstant-p[i];
    } else {
      Delta[i] = damping*(p[i]+nghSum[i])+addedConstant-p[i];
    }

    p[i] += Delta[i];
    if(start) Delta[i]-=one_over_n; //hack because initial delta is messed up as p starts at 0.0
    
    return (fabs(Delta[i]) > epsilon2 * p[i]);
  }
};

struct PR_Vertex_Reset {
  double* nghSum;
  PR_Vertex_Reset(double* _nghSum) :
    nghSum(_nghSum) {}
  inline bool operator () (intT i) {
    nghSum[i] = 0.0;
    return 1;
  }
};


timer t1,t2,t0;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  startTime();
  bool denseWrite = 1;
  const intT n = GA.n;
  const double damping = 0.85;
  const double epsilon = 0.0000001;
  const double epsilon2 = 0.01;

  double one_over_n = 1/(double)n;
  double* p = newA(double,n);
  parallel_for(intT i=0;i<n;i++) p[i] = 0.0;//one_over_n;
  double* Delta = newA(double,n);
  parallel_for(intT i=0;i<n;i++) Delta[i] = one_over_n;
  double* nghSum = newA(double,n);
  parallel_for(intT i=0;i<n;i++) nghSum[i] = 0.0;
  bool* frontier = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) frontier[i] = 1;

  vertices Frontier(n,n,frontier);
  bool* all = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) all[i] = 1;

  vertices All(n,n,all);

  intT round = 0;
  timer t1,t2;

  intT numChanged = n;
  while(1){
    round++;
    if(round > 1000) { cout << "too many rounds\n"; abort(); }
    t1.start();
    cout<<"Round "<<round<<", frontier size = "<<Frontier.numNonzeros()<<endl;
    //for(intT i=0;i<min(n,10);i++)cout<<p[i]<<" ";cout<<endl;
    vertices output = edgeMap(GA, Frontier, PR_Add(GA.V,Delta,nghSum),GA.m/20,denseWrite);
    output.del();
    t1.stop();
    t2.start();

    //cout<<"nghSum:";for(intT i=0;i<min(n,10);i++)cout<<nghSum[i]<<" ";cout<<endl;

    vertices active = vertexFilter(All,PR_Vertex_F(p,Delta,nghSum,damping,one_over_n,epsilon2));
    //for(intT i=0;i<min(n,10);i++)cout<<p[i]<<" ";cout<<endl;
    //cout<<"Delta:";for(intT i=0;i<min(n,10);i++)cout<<Delta[i]<<" ";cout<<endl;
    //cout<<"sum of p's = "<<sequence::plusReduce(p,n)<<endl;
    //cout<<"num active = "<<active.numNonzeros()<<endl;
    t2.stop();
    t0.start();
    //compute L1-norm (use nghSum as temp array)
    parallel_for(intT i=0;i<n;i++) {
      nghSum[i] = fabs(Delta[i]);
    }
    double L1_norm = sequence::plusReduce(nghSum,n);
    cout<<"L1 norm = "<<L1_norm<<endl;
    t0.stop();

    if(L1_norm < epsilon) break;
    t0.start();
    //reset
    vertexMap(All,PR_Vertex_Reset(nghSum));
    t0.stop();
    Frontier.del();
    Frontier = active;
  }
  cout<<"Finished in "<<round<<" iterations\n";
  Frontier.del();
  free(p); free(Delta); free(nghSum);
  All.del();
  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("vertexFilter");
  t0.reportTotal("compute error, reset");
  nextTime("Total");

  return make_pair(0,0);
}
