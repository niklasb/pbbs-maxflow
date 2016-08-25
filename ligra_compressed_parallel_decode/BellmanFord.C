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

//need to define this for weighted graph applications before including
//ligra.h
#define WEIGHTED 1 

#include "ligra.h"
#include "gettime.h"
#include "parseCommandLine.h"
using namespace std;

//Update ShortestPathLen if found a shorter path
struct BellmanFord_F {
  intE* ShortestPathLen;
  int* Visited;
  BellmanFord_F(intE* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool update(const uintE &s, const uintE &d, const intE &edgeLen) 
  { intE newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}}
    return 0; }
  inline bool updateAtomic(const uintE &s, const uintE &d, const intE &edgeLen) 
  { intE newDist = ShortestPathLen[s] + edgeLen;
    return (writeMin(&ShortestPathLen[d],newDist) &&
	    CAS(&Visited[d],0,1));}
  inline bool cond(const uintE &i) { return cond_true(i);}};

//reset visited vertices
struct BF_Vertex_F {
  int* Visited;
  BF_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (const uintE &i){
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
intE* BellmanFord(uintE start, graph<vertex> GA) {
  startTime();
  long n = GA.n;

  //initialize ShortestPathLen to "infinity"
  intE* ShortestPathLen = newA(intE,n);
  {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = INT_MAX/2;}
  ShortestPathLen[start] = 0;

  int* Visited = newA(int,n);
  {parallel_for(long i=0;i<n;i++) Visited[i] = 0;}

  vertices Frontier(n,start); //initial frontier

  long round = 0;
  while(!Frontier.isEmpty()){
    round++;
    if(round == n) {
      //negative weight cycle
      {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = -(INT_MAX/2);}
      break;
    }
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    vertices output = 
      edgeMap(GA, Frontier, BellmanFord_F(ShortestPathLen,Visited),GA.m/20);
    vertexMap(output,BF_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
  } 
  Frontier.del(); free(Visited);
  nextTime("Bellman Ford");
  return ShortestPathLen;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  long start = P.getOptionLongValue("-r",0);
  if(symmetric) {
    graph<symmetricVertex> WG = 
      readGraph<symmetricVertex>(iFile,symmetric);
    intE* ShortestPaths = BellmanFord((uintE)start,WG);
    free(ShortestPaths);
    WG.del(); 
  } else {
    graph<asymmetricVertex> WG = 
      readGraph<asymmetricVertex>(iFile,symmetric);
    intE* ShortestPaths = BellmanFord((uintE)start,WG);
    free(ShortestPaths);
    WG.del();
  }
}
