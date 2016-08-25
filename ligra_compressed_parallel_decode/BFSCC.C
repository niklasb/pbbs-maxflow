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
#include "parallel.h"
#include "parseCommandLine.h"
using namespace std;

//This implementation uses BFS's to find connected components until
//all vertices have been visited. This will work well for dense
//low-diameter graphs. Do not use it on very high-diameter graphs or
//graphs with very many components.

struct BFS_F {
  uintE* Parents;
  uintE label;
  BFS_F(uintE* _Parents, uintE _label) : Parents(_Parents), label(_label) {}
  inline bool update(const uintE &s, const uintE& d) {
    if(Parents[d] == UINT_E_MAX) { Parents[d] = label; return 1; }
    else return 0; }
  inline bool updateAtomic(const uintE &s, const uintE &d) {
    return (CAS(&Parents[d],UINT_E_MAX,label)); }
  //cond function checks if vertex has been visited yet
  inline bool cond(const uintE &d) { return Parents[d] == UINT_E_MAX;}};

template <class vertex>
void BFS(graph<vertex> GA) {
  startTime();
  long n = GA.n;
  //creates Parents array, initialized to all infinity, except for start
  uintE* Parents = newA(uintE,GA.n);
  parallel_for(long i=0;i<n;i++) Parents[i] = UINT_E_MAX;
  long numVisited = 0;

  for(long i=0;i<n;i++) {
    intT start = i;
    if(Parents[start] == UINT_E_MAX) {
      Parents[start] = start;
      vertices Frontier(n,start); //creates initial frontier
      long round = 0;
      while(!Frontier.isEmpty()){ //loop until frontier is empty
	round++;
	numVisited+=Frontier.numNonzeros();
	//apply edgemap
	vertices output = edgeMap(GA, Frontier, BFS_F(Parents,start), GA.m/20);    
	Frontier.del();
	Frontier = output; //set new frontier
      } 
      Frontier.del(); 
      if(numVisited == n) break;
    }
  }
  free(Parents);
  //cout << "rounds: " << round << endl;
  cout<<"Vertices visited = "<<numVisited<<endl;
  // cout<<"Edges traversed = "<<edgesTraversed<<endl;
  nextTime("BFS");
}

int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  long start = P.getOptionLongValue("-r",0);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric); //symmetric graph
    BFS(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric); //asymmetric graph
    BFS(G);
    G.del();
  }
}
