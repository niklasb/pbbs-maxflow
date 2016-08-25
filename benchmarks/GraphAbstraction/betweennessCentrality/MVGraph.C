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

typedef double fType;

#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "Abstract.h"
#include "gettime.h"
#include <vector>
#include "math.h"
#include "BCBFS.h"
using namespace std;


void paths(intT start, fType* ActualNumPaths, graph<intT> G){
  intT* Q = newA(intT,G.n);
  intT start_ptr = 0, end_ptr = 0;
  Q[end_ptr++] = start;
  intT* L = newA(intT,G.n);
  parallel_for(intT i=0;i<G.n;i++) L[i] = -1;
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
	  ActualNumPaths[ngh] += ActualNumPaths[v];
	} else if(L[ngh] == level){
	  ActualNumPaths[ngh] += ActualNumPaths[v];
	}
      }
    }
    if(end_ptr == next_end_ptr) break;
    start_ptr = end_ptr; 
    end_ptr = next_end_ptr;
  }
  
  free(Q);
  free(L);
}


void BCCheck(intT start, graph<intT> GA, fType* NumPaths){
  intT n = GA.n;
  fType* ActualNumPaths = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) ActualNumPaths[i] = 0.0;
  ActualNumPaths[start] = 1.0;

  paths(start,ActualNumPaths,GA);
  fType epsilon = 0.00001;  
  for(intT i=0;i<n;i++){
    if(fabs((NumPaths[i] - ActualNumPaths[i])/ActualNumPaths[i]) > epsilon){
      //cout<<fabs(NumPaths[i]-ActualNumPaths[i])<<endl;
      cout<<"NumPaths["<<i<<"] wrong "<<NumPaths[i]<<" "
	  <<ActualNumPaths[i]<<endl;
      abort();
    }
  }
  free(ActualNumPaths);
}

void BCCheck2(intT start, graph<intT> GA, fType* Dependencies){
  intT n = GA.n;
  fType* NumPaths = newA(fType,n);
  intT* Visited = newA(intT,n);
  pair<pair<intT*,intT*>, intT> L = BCBFS(start,GA,Visited,NumPaths);
  intT numLevels = L.second;
  intT* Levels = L.first.first;
  intT* LevelOffsets = L.first.second;

  fType* ActualDependencies = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) ActualDependencies[i] = 0.0;

  //process nodes from furthest to closest
  for(intT i=numLevels-2;i>=0;i--){
    intT start = LevelOffsets[i];
    intT end = LevelOffsets[i+1];
    parallel_for(intT j = start; j < end; j++){
      intT v = Levels[j];
      //process nodes at i'th level
      for(intT k=0;k<GA.V[v].degree;k++){
	intT ngh = GA.V[v].Neighbors[k];
	if(Visited[ngh] == i+1){ //level above
	  ActualDependencies[v] += 
	    (NumPaths[v]/NumPaths[ngh])
	    *(1+ActualDependencies[ngh]);
	}
      }
    }
  }
  free(Levels); free(LevelOffsets); free(NumPaths); free(Visited);
  fType epsilon = 0.00001;
  for(intT i=0;i<n;i++){
    if(fabs((Dependencies[i] - ActualDependencies[i])/ActualDependencies[i])
       > epsilon){
      cout<<"Dependencies["<<i<<"] wrong "<<Dependencies[i]
	  <<" "<<ActualDependencies[i]<<endl;
      abort();
    }
  }
  free(ActualDependencies);  
}

struct BC_Add {
  fType* NumPaths;
  bool* Visited;

  BC_Add(fType* _NumPaths, bool* _Visited) : 
    NumPaths(_NumPaths), Visited(_Visited) {}
  inline bool add(intT s, intT d){
    fType oldV = NumPaths[d];
    NumPaths[d] += NumPaths[s];
    return oldV == 0.0;
  }
  inline bool addAtomic (intT s, intT d) {
    volatile fType oldV, newV; 
    do {
      oldV = NumPaths[d]; newV = oldV + NumPaths[s];
    } while(!utils::CAS(&NumPaths[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; }
};

struct BC_Add_Back {
  fType* Dependencies;
  bool* Visited;
  BC_Add_Back(fType* _Dependencies, bool* _Visited) : 
    Dependencies(_Dependencies), Visited(_Visited) {}
  inline bool add(intT s, intT d){
    fType oldV = Dependencies[d];
    Dependencies[d] += Dependencies[s];
    //Dependencies[d] += ((NumPaths[d]/NumPaths[s])*(1+Dependencies[s]));
    return oldV == 0.0;
  }
  inline bool addAtomic (intT s, intT d) {
    volatile fType oldV, newV;
    do {
      oldV = Dependencies[d];
      //newV = oldV + (NumPaths[d]/NumPaths[s])*(1+Dependencies[s]);
      newV = oldV + Dependencies[s];
    } while(!utils::CAS(&Dependencies[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; }
};

struct BC_Vertex_F {
  bool* Visited;
  BC_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    return 1;
  }
};

struct BC_Back_Vertex_F {
  bool* Visited;
  fType* Dependencies, *inverseNumPaths;
  BC_Back_Vertex_F(bool* _Visited, fType* _Dependencies, fType* _inverseNumPaths) : 
    Visited(_Visited), Dependencies(_Dependencies), inverseNumPaths(_inverseNumPaths) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    Dependencies[i] += inverseNumPaths[i];
    return 1;
  }
};

timer t1,t2,t0;

pair<intT,intT> MVGraph(intT start, graph<intT> GA) {
  startTime();
  bool denseForward = 0;
  intT n = GA.n;
  intT threshold = GA.m/20;

  fType* NumPaths = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) NumPaths[i] = 0.0;
  NumPaths[start] = 1.0;

  bool* Visited = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;
  Visited[start] = 1;
  vertices Frontier(n,start);
 
  vector<vertices> Levels;
  Levels.push_back(Frontier);

  intT round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros();
    t1.start();
    vertices output = edgeMap(GA, Frontier, BC_Add(NumPaths,Visited),threshold,denseForward);
    t1.stop();
    t2.start();
    vertexMap(output, BC_Vertex_F(Visited));
    t2.stop();
    Levels.push_back(output);
    Frontier = output;
  }
  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("Merging output");
  nextTime("Total");

  //BCCheck(start,GA,NumPaths);
 
  t0.start();
  fType* Dependencies = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) Dependencies[i] = 0.0;

  //invert numpaths
  fType* inverseNumPaths = NumPaths;
  parallel_for(intT i=0;i<n;i++) inverseNumPaths[i] = 1/inverseNumPaths[i];

  Levels[round].del();
  //reuse Visited
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;
  Frontier = Levels[round-1];
  vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));

  //for(intT i=0;i<n;i++)cout<<NumPaths[i]<<" ";cout<<endl;
  
  for(intT r=round-2;r>=0;r--) {
    vertices output = edgeMap(GA, Frontier, 
			      BC_Add_Back(Dependencies,Visited),threshold,denseForward);
    output.del();
    Frontier.del();
    Frontier = Levels[r];
    vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));
  }
  Frontier.del();
  t0.reportTotal("Backwards phase");

  //for(intT i=0;i<n;i++) cout<<Dependencies[i]<<" ";cout<<endl;

  parallel_for(intT i=0;i<n;i++) {
    Dependencies[i]=(Dependencies[i]-inverseNumPaths[i])/inverseNumPaths[i];
  }
  free(inverseNumPaths);

  free(Visited);
  //BCCheck2(start,GA,Dependencies);
  free(Dependencies);
  return make_pair(0,0);
}
