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
#include "sequence.h"
using namespace std;

// **************************************************************
//    PARALLEL BELLMAN-FORD
// **************************************************************

//Basic version where every vertex tries to propagate updates
bool relax(wghGraph<intT> GA, intT* ShortestPathLen){
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  wghVertex<intT> *G = GA.V;
  bool* nextFrontier = newA(bool,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) nextFrontier[i] = 0;

  parallel_for(intT i=0;i<numVertices;i++){
    for(intT j=0;j<G[i].degree;j++){
      intT ngh = G[i].Neighbors[j];
      intT edgeLength = G[i].nghWeights[j];
      intT newDist = ShortestPathLen[i] + edgeLength;
      if(utils::writeMin(&ShortestPathLen[ngh],newDist)){ 
	nextFrontier[i] = 1;
      }
    }
  }
  
  bool r = sequence::sum(nextFrontier,numVertices) > 0;
  free(nextFrontier);
  return r;
}

//Only vertices whose shortest path length changed in the previous iteration need to propagate. Non-deterministic because vertices may propagate values received in current iteration.

//Dense frontier version (write-based)
pair<bool,bool*> relax_dense_forward(wghGraph<intT> GA, intT* ShortestPathLen, bool* Frontier){
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  wghVertex<intT> *G = GA.V;
  bool* nextFrontier = newA(bool,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) nextFrontier[i] = 0;
  parallel_for(intT i=0;i<numVertices;i++){
    if(Frontier[i]) {
      for(intT j=0;j<G[i].degree;j++){
	intT ngh = G[i].Neighbors[j];
	intT edgeLength = G[i].nghWeights[j];
	intT newDist = ShortestPathLen[i] + edgeLength;
	if(utils::writeMin(&ShortestPathLen[ngh],newDist)){ 
	  if(nextFrontier[ngh] == 0) nextFrontier[ngh] = 1;
	}
      }
    }
  }

  bool r = sequence::sum(nextFrontier,numVertices) > 0;
  return make_pair(r,nextFrontier);
}

//Dense frontier version (read-based)
pair<bool,bool*> relax_dense_read(wghGraph<intT> GA, intT* ShortestPathLen, bool* Frontier){
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  wghVertex<intT> *G = GA.V;
  bool* nextFrontier = newA(bool,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) nextFrontier[i] = 0;
  parallel_for(intT i=0;i<numVertices;i++){    
    for(intT j=0;j<G[i].degree;j++){
      intT ngh = G[i].Neighbors[j];
      if(Frontier[ngh]){
	intT edgeLength = G[i].nghWeights[j];
	intT newDist = ShortestPathLen[ngh] + edgeLength;
	if(ShortestPathLen[i] > newDist){
	  ShortestPathLen[i] = newDist;
	  if(!nextFrontier[i]) nextFrontier[i] = 1;
	}
      } 
    }
  }

  bool r = sequence::sum(nextFrontier,numVertices) > 0;
  return make_pair(r,nextFrontier);
}

//Sparse frontier version
struct nonNegF{bool operator() (intT a) {return (a>=0);}};

intT relax_sparse(wghGraph<intT> GA, intT* ShortestPathLen, intT* Frontier, intT frontierSize, intT* NextFrontier){
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  wghVertex<intT> *G = GA.V;

  intT* Counts = newA(intT,frontierSize);
  parallel_for(intT i=0;i<frontierSize;i++) Counts[i] = G[Frontier[i]].degree;

  intT totalDegree = sequence::plusScan(Counts,Counts,frontierSize);

  int* Visited = newA(int,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) Visited[i] = 0;

  parallel_for(intT i=0;i<frontierSize;i++){
    intT v = Frontier[i];
    intT o = Counts[i]; 
    for(intT j=0;j<G[v].degree;j++){
      intT ngh = G[v].Neighbors[j];
      intT edgeLength = G[v].nghWeights[j];
      intT newDist = ShortestPathLen[v] + edgeLength;
      if(utils::writeMin(&ShortestPathLen[ngh],newDist) && 
	 utils::CAS(&Visited[ngh],0,1)){ 
	NextFrontier[o+j] = ngh;
      } else NextFrontier[o+j] = -1;
    }
  }
  free(Visited);
  free(Counts);

  intT newFrontierSize = sequence::filter(NextFrontier,Frontier,totalDegree,nonNegF());

  return newFrontierSize;
}


intT* bellmanFord(intT start, wghGraph<intT> GA) {
  intT numVertices = GA.n;
  intT numEdges = GA.m;

  intT* ShortestPathLen = newA(intT,numVertices);
  parallel_for (intT i = 0; i < numVertices; i++) 
    ShortestPathLen[i] = INT_T_MAX/2;
  ShortestPathLen[start] = 0;
  bool* Frontier = newA(bool,numVertices);
  parallel_for(intT i=0;i<numVertices;i++) Frontier[i] = 0;
  Frontier[start] = 1;
  intT round = 0;

  intT* SFrontier = newA(intT,numEdges);
  intT* SFrontierNext = newA(intT,numEdges);
  SFrontier[0] = start;
  intT frontierSize = 1;

  bool switching = 0;
  const intT threshold = numVertices/20;
  for(intT i=0;i<numVertices-1;i++) {
    round++;
    //cout << "Round "<<round<<" frontier size = "<<frontierSize;
    if(frontierSize > threshold){
      //cout<<" dense\n";
      if(switching) {
	parallel_for(intT i=0;i<numVertices;i++) Frontier[i] = 0;
	parallel_for(intT i=0;i<frontierSize;i++)
	  Frontier[SFrontier[i]] = 1;
	switching = 0;
      }
      pair<bool,bool*> result = relax_dense_forward(GA,ShortestPathLen,Frontier);
      if(!result.first) break;
      free(Frontier);
      Frontier = result.second; 
      frontierSize = sequence::sum(Frontier,numVertices);
      if(frontierSize < threshold) switching = 1;
    } else {
      //cout<<" sparse\n";
      if(switching) {
	_seq<intT> R = sequence::packIndex(Frontier,numVertices);
	parallel_for(intT i=0;i<frontierSize;i++)
	  SFrontier[i] = R.A[i];
	R.del();
	switching = 0;
      }
      intT result = relax_sparse(GA,ShortestPathLen,SFrontier,frontierSize,SFrontierNext);

      if(!result) break;
      else { frontierSize = result; }
      if(frontierSize > threshold) switching = 1;
    }
  }
  free(Frontier);

  //check for negative weight cycle
  if(round==numVertices-1 && relax(GA,ShortestPathLen))
    parallel_for(intT i=0;i<numVertices;i++) 
      ShortestPathLen[i] = -(INT_T_MAX/2);
  // else cout <<"Finished in "<<round<<" iterations\n"; 
  
  //for(intT i=0;i<numVertices;i++) cout<<i<<" : "<<ShortestPathLen[i]<<endl;

  return ShortestPathLen; 
}
