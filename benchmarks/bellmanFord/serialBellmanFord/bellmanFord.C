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

#include "graph.h"
#include "utils.h"
#include "parallel.h"
using namespace std;

// **************************************************************
//    SERIAL BELLMAN-FORD
// **************************************************************

bool relax(wghGraph<intT> GA, intT* ShortestPathLen){
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  wghVertex<intT> *G = GA.V;
  bool changed = 0;
  for(intT i=0;i<numVertices;i++){
    for(intT j=0;j<G[i].degree;j++){
      intT ngh = G[i].Neighbors[j];
      intT edgeLength = G[i].nghWeights[j];
      if(ShortestPathLen[ngh] > ShortestPathLen[i]+edgeLength){
	ShortestPathLen[ngh] = ShortestPathLen[i]+edgeLength;
	changed = 1;
      }
    }
  }
  return changed;
}

intT* bellmanFord(intT start, wghGraph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;

  intT* ShortestPathLen = newA(intT,numVertices);
  for (intT i = 0; i < numVertices; i++) 
    ShortestPathLen[i] = INT_T_MAX/2;
  ShortestPathLen[start] = 0;
  intT round = 0;
  for(intT i=0;i<numVertices-1;i++) {
    round++;
    if(!relax(GA,ShortestPathLen)) break;
  }

  //check for negative weight cycle
  if(round==numVertices-1 && relax(GA,ShortestPathLen))
    for(intT i=0;i<numVertices;i++) ShortestPathLen[i] = -(INT_T_MAX/2);
  //else cout <<"Finished in "<<round<<" iterations\n"; 
    
  //for(intT i=0;i<numVertices;i++) cout<<i<<" : "<<ShortestPathLen[i]<<endl;

  return ShortestPathLen;
}
