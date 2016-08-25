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
#include "radii.h"
using namespace std;

timer t0;
void radiiCheck(graph<intT> GA, intT* radii){
  t0.start();
  intT* ActualRadii = BFS64_hybrid(GA);
  t0.reportTotal("checker time");
  for(intT i=0;i<GA.n;i++) {
    if(ActualRadii[i] != radii[i]){
      cout<<"radii["<<i<<"] wrong "<<radii[i]<<" "<<ActualRadii[i]<<endl;
      abort();
    }
  }
  free(ActualRadii);
}


template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !utils::CAS(a, oldV, newV));
}

struct Radii_Add {
  intT round;
  intT* radii;
  long* Visited, *NextVisited;
  Radii_Add(long* _Visited, long* _NextVisited, intT* _radii, intT _round) : 
    Visited(_Visited), NextVisited(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool add (intT s, intT d){
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      NextVisited[d] |= toWrite;
      if(radii[d] != round) { radii[d] = round; return 1; }
    }
    return 0;
  }
  inline bool addAtomic (intT s, intT d){
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      writeOr(&NextVisited[d],toWrite);
      intT oldRadii = radii[d];
      if(radii[d] != round) return utils::CAS(&radii[d],oldRadii,round);
    }
    return 0;
  }
  inline bool cond (intT d) { return 1; }
};

struct Radii_Vertex_F {
  long* Visited, *NextVisited;
  Radii_Vertex_F(long* _Visited, long* _NextVisited) :
    Visited(_Visited), NextVisited(_NextVisited) {}
  inline bool operator() (intT i) {
    NextVisited[i] |= Visited[i];
    return 1;
  }
};

timer t1,t2,t5;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {

  startTime();
  bool denseForward = 0;
  intT n = GA.n;

  intT* radii = newA(intT,n);
  long* Visited = newA(long,n);
  long* NextVisited = newA(long,n);
  parallel_for(intT i=0;i<n;i++) {
    radii[i] = -1;
    Visited[i] = NextVisited[i] = 0;
  }
  intT sampleSize = min<intT>(n,64);
  intT* starts = newA(intT,sampleSize);
  parallel_for(intT i=0;i<sampleSize;i++) { 
    radii[i] = 0;
    starts[i] = i; 
    Visited[i] = (long) 1<<i;
  }
  vertices Frontier(n,sampleSize,starts);

  intT round = 0;
  timer t1;

  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t5.start();
    vertexMap(Frontier, Radii_Vertex_F(Visited,NextVisited));
    t5.stop();
    t1.start();
    vertices output = edgeMap(GA, Frontier, Radii_Add(Visited,NextVisited,radii,round),GA.m/20,denseForward);
    t1.stop();
    swap(NextVisited,Visited);
    Frontier.del();
    Frontier = output;
  }
  free(Visited); free(NextVisited);
  
  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t5.reportTotal("copy");
  t1.reportTotal("MVMult");
  Frontier.del();
  nextTime("total radii time");
  
  //radiiCheck(GA,radii);

  free(radii); 

  return make_pair(0,0);
}
