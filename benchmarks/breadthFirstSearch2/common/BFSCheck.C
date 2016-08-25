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

#include <iostream>
#include <algorithm>
#include <cstring>
#include "parallel.h"
#include "IO.h"
#include "graph.h"
#include "graphIO.h"
#include "parseCommandLine.h"
using namespace std;
using namespace benchIO;


struct getA {
  intT* A;
  getA(intT* AA) : A(AA) {}
  intT operator() (intT i) {return (A[i] == -1) ? 0 : 1;}
};

struct notMinusOne {bool operator() (intT i) {return (-1 != i);}};

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

  //check that number of vertices touched is equal to number of edges
  //minus 1
  intT visited = sequence::reduce<intT>((intT)0,G.n,utils::addF<intT>(),getA(L));
  intT numParents = sequence::reduce<intT>((intT)0,G.n,utils::addF<intT>(),getA(Parents));
  if(visited != numParents) { cout << "Result not a BFS tree: " << visited << " vertices visited, but only " << numParents-1<<" edges\n";}


  free(L);
 
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<inFile> <outfile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;

  graph<intT> G = readGraphFromFile<intT>(iFile);
  _seq<intT> Output = readIntArrayFromFile<intT>(oFile);
  intT* Parents = Output.A;
  checkBFS(0, G, Parents);
}
