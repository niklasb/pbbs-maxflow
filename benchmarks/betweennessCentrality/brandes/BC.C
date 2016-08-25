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
#include "BFS.h"
using namespace std;

// **************************************************************
//    Betweenness Centrality
// **************************************************************

void printGraph(graph<intT> GA){
  for(int i=0;i<GA.n;i++){
    cout<<i<<":";
    for(int j=0;j<GA.V[i].degree;j++){
      cout<<GA.V[i].Neighbors[j]<<" ";
    }
    cout<<endl;
  }
}

#define sampleSize 10
#define SCALE 1

void BC(graph<intT> GA){
  //printGraph(GA);
  vertex<intT>* V = GA.V;
  intT n = GA.n;
  cout<<"n = "<<n<<" numSamples = "<<sampleSize<<endl;
  double* Dependencies = newA(double,n);
  parallel_for(intT i=0;i<n;i++) Dependencies[i] = 0;

  double* NumPaths = newA(double,n);
  intT* Visited= newA(intT,n);
  for(intT i=0;i<min(n,sampleSize);i++){
    pair<pair<intT*,intT*>, intT> L = BFS(utils::hash(i)%n,GA,Visited,NumPaths);
    intT numLevels = L.second;
    intT* Levels = L.first.first;
    intT* LevelOffsets = L.first.second;

    //for(int i=0;i<n;i++)cout<<"("<<Levels[i]<<","<<LevelOffsets[i]<<") ";cout<<endl;
    //for(int i=0;i<n;i++)cout<<Visited[i]<<" ";cout<<endl;
    // cout<<endl;

    //cout<<"num rounds = "<<numLevels<<endl;

    //for(intT i=0;i<n;i++)cout<<NumPaths[i]<<" ";cout<<endl;

    if(numLevels < n && LevelOffsets[numLevels] == -1){
      cout<<"some unreachable\n";
    }


    //process nodes from furthest to closest
    for(intT i=numLevels-2;i>=0;i--){
      intT start = LevelOffsets[i];
      intT end = LevelOffsets[i+1];
      parallel_for(intT j = start; j < end; j++){
	intT v = Levels[j];
	//process nodes at i'th level
	for(intT k=0;k<V[v].degree;k++){
	  intT ngh = V[v].Neighbors[k];
	  if(Visited[ngh] == i+1){ //level above
	    Dependencies[v] += 
	      ((double)(NumPaths[v]/NumPaths[ngh]))*(1+Dependencies[ngh]);
	  }
	}
	//cout<<" dependency = "<<Dependencies[v];

      }
      //cout<<endl;
    }
    free(LevelOffsets); free(Levels);
  }
  //scaling

  if(SCALE)
    parallel_for(intT i=0;i<n;i++) Dependencies[i]*=(double)n/sampleSize;
  
  for(intT i=0;i<min(n,1000);i++)cout<<Dependencies[i]<<" ";cout<<endl;

  free(NumPaths); free(Dependencies);
  free(Visited);
}
