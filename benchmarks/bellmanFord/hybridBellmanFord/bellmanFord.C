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
#include "WghAbstract.h"
#include "gettime.h"
using namespace std;

struct BF_Add {
  intT* ShortestPathLen;
  int* Visited;
  BF_Add(intT* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool add (intT s, intT d, intT edgeLen) {
    intT newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool addAtomic (intT s, intT d, intT edgeLen){
    intT newDist = ShortestPathLen[s] + edgeLen;
    return (utils::writeMin(&ShortestPathLen[d],newDist) &&
	    utils::CAS(&Visited[d],0,1));
  }
  inline bool cond (intT d) { return 1; }
};

struct BF_Vertex_F {
  int* Visited;
  BF_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (intT i){
    Visited[i] = 0;
    return 1;
  }
};

timer t1,t2;

intT* bellmanFord(intT start, wghGraph<intT> GA) {
  
  startTime();
  bool denseForward = 1;
  intT n = GA.n;

  intT* ShortestPathLen = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) ShortestPathLen[i] = INT_T_MAX/2;
  ShortestPathLen[start] = 0;

  int* Visited = newA(int,n);
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;

  vertices Frontier(n,start);

  intT round = 0;
  timer t1;

  while(!Frontier.isEmpty()){
    round++;
    if(round == n) {
      //negative weight cycle
      parallel_for(intT i=0;i<n;i++) ShortestPathLen[i] = -(INT_T_MAX/2);
      break;
    }
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;

    t1.start();
    vertices output = edgeMap(GA, Frontier, BF_Add(ShortestPathLen,Visited), GA.m/20,denseForward);
    t1.stop();
    t2.start();
    vertexMap(output,BF_Vertex_F(Visited));
    t2.stop();
    Frontier.del();
    Frontier = output;
  } 

  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t2.reportTotal("reset");
  t1.reportTotal("MVMult");
  Frontier.del();
  nextTime("total hybrid time");
  free(Visited);
  return ShortestPathLen;
}
