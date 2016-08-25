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
#include <stdio.h>
#include <assert.h>
#include "Abstract.h"
using namespace std;

// **************************************************************
//    HYBRID BREADTH FIRST SEARCH
// **************************************************************

// **************************************************************
//    Switches to Dense matrix-vector multiply when frontier size
//    is large enough
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



// struct LevelInfo {
//   intT* Levels;
//   intT* LevelOffsets;
//   intT* LevelVals;
//   intT numLevels;
//   LevelInfo(intT* _Levels,intT* _LevelOffsets, intT* _LevelVals, intT _numLevels) :
//     Levels(_Levels), LevelOffsets(_LevelOffsets), LevelVals(_LevelVals), numLevels(_numLevels) {}
// };



// LevelInfo BC_Phase1(graph<intT> GA, intT* Frontier, intT initFrontierSize, double* NumPaths) {
//   intT numVertices = GA.n;
//   intT numEdges = GA.m;
//   vertex<intT> *G = GA.V;
//   intT* NextVisited = newA(intT,numVertices);
//   intT* Visited = newA(intT,numVertices);
//   intT* FrontierNext = newA(intT,numEdges);
//   intT* Counts = newA(intT,numVertices);
//   intT* Levels = newA(intT,numVertices);
//   intT* LevelOffsets = newA(intT,numVertices+1);
//   intT* LevelVals = newA(intT,numVertices);


//   {parallel_for(intT i = 0; i < numVertices; i++) {
//       Visited[i] = 0;
//       NextVisited[i] = 0;
//       Counts[i] = 0;
//       NumPaths[i] = 0;
//       LevelVals[i] = -1;
//     }
//   }
//   intT frontierSize = initFrontierSize;
//   intT lastFrontierSize = 0;
//   parallel_for(intT i=0;i<frontierSize;i++){
//     intT v = Frontier[i];
//     Visited[v] = NextVisited[v] = NumPaths[v] = 1;
//     Levels[i] = 0;
//     LevelVals[v] = 0;
//   }
//   LevelOffsets[0] = 0;
//   LevelOffsets[1] = frontierSize;

//   intT totalVisited = 0;
//   int round = 0;
//   int threshold = (numVertices+numEdges)/10;
//   cout<<"threshold = "<<threshold<<endl;
//   bool fromSMV = 0;

//   {parallel_for (intT i=0; i < frontierSize; i++) 
//       Counts[i] = G[Frontier[i]].degree;}
//   intT frontierEdgeCount = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);

//   while (frontierSize > 0) {
//     LevelOffsets[round] = totalVisited;
//     round++;

//     if(frontierSize+frontierEdgeCount< threshold){
//       totalVisited += frontierSize;
//       if(fromSMV){
// 	fromSMV = 0;
// 	pair<intT,intT> frontierInfo = getFrontierInfo(GA,Frontier,Visited,NextVisited,Counts);
// 	frontierSize = frontierInfo.first;
// 	frontierEdgeCount = frontierInfo.second;
//       }

//       cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-sparse method\n";

//       // For each vertexB in the frontier try to "hook" unvisited neighbors.
//       {parallel_for(intT i = 0; i < frontierSize; i++) {
// 	  intT v = Frontier[i];
// 	  intT o = Counts[i];
// 	  // Levels[totalVisited+i] = v;
// 	  // LevelVals[v] = round-1;

// 	  for (intT j=0; j < G[v].degree; j++) {
// 	    intT ngh = G[v].Neighbors[j];
// 	    if (NextVisited[ngh] == 0 && utils::CAS(&NextVisited[ngh],(intT)0,(intT)1)) 
// 	      {
// 		FrontierNext[o+j] = ngh; 
// 	      }
// 	    else FrontierNext[o+j] = -1;
// 	    if(Visited[ngh] == 0) utils::writeAdd(&NumPaths[ngh],NumPaths[v]);
// 	  }
// 	}}

//       parallel_for(intT i=0;i<frontierSize;i++) {
// 	intT v = Frontier[i];
// 	for(intT j=0;j<G[v].degree;j++){
// 	  intT ngh = G[v].Neighbors[j];
// 	  if(Visited[ngh] == 0) Visited[ngh] = 1;
// 	}
//       }
      
	
//       // Filter out the empty slots (marked with -1)
//       lastFrontierSize = frontierSize;
//       frontierSize = sequence::filter(FrontierNext,Frontier,frontierEdgeCount,nonNegF());

//       parallel_for(intT i=0;i<frontierSize;i++){
// 	intT v = Frontier[i];
// 	Levels[totalVisited+i] = v;
// 	LevelVals[v] = round;
//       }

//       parallel_for(intT i=0;i<frontierSize;i++)
//       	  Counts[i]=G[Frontier[i]].degree;
//       frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);

//     }
//     else {
//       //spsv
//       cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n";

//       totalVisited+=frontierSize;
//   // cout<<"visited: ";
//   // for(intT i=0;i<GA.n;i++)cout<<Visited[i]<<" ";cout<<endl;
//       parallel_for(intT i=0;i<numVertices;i++){
// 	NextVisited[i] = Visited[i];
// 	if(!NextVisited[i]){
// 	  for(intT j=0;j<G[i].degree;j++){
// 	    intT ngh = G[i].Neighbors[j];
// 	    // NextVisited[i] |= Visited[ngh];
// 	    // if(Parents[i] != -1) Parents[i] = ngh;
// 	    if(Visited[ngh]) {
// 	      NumPaths[i] += NumPaths[ngh];
// 	      NextVisited[i] = 1;

// 	      if(LevelVals[i] == -1) LevelVals[i] = round;
// 	      //break;
// 	    }
// 	  }
// 	}
//       }




//       frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,NextVisited));
      
//       frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierSize(G,Visited,NextVisited));


//       //fill in Levels array
//       parallel_for(intT i=0;i<numVertices;i++){
// 	if(LevelVals[i] == round) Counts[i] = 1;
// 	else Counts[i] = 0;
//       }
      
//       intT frontierSize = sequence::scan(Counts,Counts,numVertices,utils::addF<intT>(),(intT)0);

//       parallel_for(intT i=0;i<numVertices-1;i++){
// 	if(Counts[i] != Counts[i+1]) Levels[totalVisited+Counts[i]] = i;
//       }
//       if(Counts[numVertices-1] == frontierSize-1)
// 	Levels[totalVisited+frontierSize-1] = numVertices-1;


      
//       if(frontierSize+frontierEdgeCount < threshold) fromSMV=1;
//       else swap(NextVisited,Visited);
//     }

//   }
//   free(FrontierNext); free(Counts);
//   free(NextVisited);

//   return LevelInfo(Levels,LevelOffsets,LevelVals,round);
//   //return pair<intT,intT>(totalVisited,lastFrontierSize);
// }

// 	    if(Visited[ngh]==0){
// 	      double toWrite = (NumPaths[ngh]/NumPaths[v])*(1+Dependencies[v]);
// 	      utils::writeAdd(&Dependencies[ngh],toWrite);
// 	    }

// 	    if(Visited[ngh]) {
// 	      Dependencies[i] += 
// 		(NumPaths[i]/NumPaths[ngh])*(1+Dependencies[ngh]);
// 	      NextVisited[i] = 1;
// 	      //break;







typedef pair<intT,intT> intPair;

struct BC_thresholdF {
  intT threshold;
  BC_thresholdF(intT _threshold) : threshold(_threshold) {}
  bool operator() (intT val) {
    return (val < threshold);
  }
};


struct BC_stoppingF {
  bool operator() (intT val) {
    return val == 0;
  }
};

struct BC_Elt {
  intPair Levels;
  double numPaths;
  BC_Elt() : numPaths(0) {}
  BC_Elt(intT i) :
    numPaths(0), Levels(intPair(i,-1)) {}
};

struct BC_Add {
  void operator() (BC_Elt& a, BC_Elt& b){
    a.numPaths += b.numPaths;
    if(a.Levels.second == -1) 
      a.Levels.second = b.Levels.second + 1;
  }
  void addAtomic (BC_Elt& a, BC_Elt& b) {
    utils::writeAdd(&a.numPaths, b.numPaths);
    if(a.Levels.second == -1)
      a.Levels.second = b.Levels.second + 1;
  }
};



pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  //printGraph(GA);

  intT* Frontier = newA(intT,GA.m);
  intT initFrontierSize = 1;
  Frontier[0] = start;
  
  //BC_Elt* BC_Vector = newA(BC_Elt,GA.n);
  BC_Elt* BC_Vector = new BC_Elt[GA.n];

  parallel_for(intT i=0;i<GA.n;i++){
    BC_Vector[i].Levels = intPair(i,-1);
  }
  BC_Vector[start].Levels.second = 0;
  BC_Vector[start].numPaths = 1;

  MV(GA,Frontier,initFrontierSize,BC_Vector,BC_Add(),
     BC_thresholdF((GA.n+GA.m)/10), BC_stoppingF());


  double* Dependencies = newA(double,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) Dependencies[i] = 0.0;
  //  BC_Phase2(GA,Frontier,lastFrontierSize,NumPaths,Dependencies);

  //do something with result

  free(Frontier);
  free(Dependencies);
  delete BC_Vector;
  return make_pair(0,0);
}
