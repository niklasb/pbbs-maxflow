// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
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
#include "ligra.h"
#include "gettime.h"
#include "math.h"
#include "parseCommandLine.h"
using namespace std;

//update function applies PageRank equation
template <class vertex>
struct PR_F {
  double* p_curr, *p_next;
  vertex* V;
  PR_F(double* _p_curr, double* _p_next, vertex* _V) : 
    p_curr(_p_curr), p_next(_p_next), V(_V) {}
  inline bool update(const uintE &s, const uintE &d){ 
    p_next[d] += p_curr[s]/V[s].getOutDegree();
    return 1;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){ 
    writeAdd(&p_next[d],p_curr[s]/V[s].getOutDegree());
    return 1;
  }
  inline bool cond(const uintE &i) {return cond_true(i);}};

//vertex map function to update its p value according to PageRank equation
struct PR_Vertex_F {
  double damping, addedConstant;
  double* p_curr, *p_next;
  PR_Vertex_F(double* _p_curr, double* _p_next, double _damping, intE n) :
    p_curr(_p_curr), p_next(_p_next), 
    damping(_damping), addedConstant((1-_damping)*(1/(double)n)){}
  inline bool operator () (const uintE &i) {
    p_next[i] = damping*p_next[i] + addedConstant;
    return 1;
  }
};

//resets p
struct PR_Vertex_Reset {
  double* p_curr;
  PR_Vertex_Reset(double* _p_curr) :
    p_curr(_p_curr) {}
  inline bool operator () (const uintE &i) {
    p_curr[i] = 0.0;
    return 1;
  }
};

template <class vertex>
void PageRank(graph<vertex> GA) {
  startTime();
  const intE n = GA.n;
  const double damping = 0.85, epsilon = 0.0000001;
  
  double one_over_n = 1/(double)n;
  double* p_curr = newA(double,n);
  {parallel_for(long i=0;i<n;i++) p_curr[i] = one_over_n;}
  double* p_next = newA(double,n);
  {parallel_for(long i=0;i<n;i++) p_next[i] = 0;} //0 if unchanged
  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;}

  vertices Frontier(n,n,frontier);
  
  long round = 0;
  while(1){
    round++;
    vertices output = 
      edgeMap(GA, Frontier, PR_F<vertex>(p_curr,p_next,GA.V),
	      GA.m/20);
    vertexMap(Frontier,PR_Vertex_F(p_curr,p_next,damping,n));
    break;

    //compute L1-norm between p_curr and p_next
    {parallel_for(long i=0;i<n;i++) {
      p_curr[i] = fabs(p_curr[i]-p_next[i]); }}
    double L1_norm = sequence::plusReduce(p_curr,n);
    //cout<<"Round "<<round<<", L1 norm = "<<L1_norm<<endl;
    if(L1_norm < epsilon) break;
    //reset p_curr
    vertexMap(Frontier,PR_Vertex_Reset(p_curr));
    swap(p_curr,p_next);
    Frontier.del(); 
    Frontier = output;
  }
  cout<<"Finished in "<<round<<" iterations\n";
  Frontier.del(); free(p_curr); free(p_next); 
  nextTime("PageRank");
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric);
    PageRank(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric);
    PageRank(G);
    G.del();
  }
}
