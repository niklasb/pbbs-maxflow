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
#include <vector>
#include "math.h"
#include "parallel.h"
#include "parseCommandLine.h"
using namespace std;

typedef double fType;

 //Update function for forward phase
struct BC_F {
  fType* NumPaths;
  bool* Visited;
  BC_F(fType* _NumPaths, bool* _Visited) : NumPaths(_NumPaths), Visited(_Visited) {}
  inline bool update(const uintE &s, const uintE &d){
    fType oldV = NumPaths[d];
    NumPaths[d] += NumPaths[s];
    return oldV == 0.0; }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    volatile fType oldV, newV; 
    do { oldV = NumPaths[d]; newV = oldV + NumPaths[s];
    } while(!CAS(&NumPaths[d],oldV,newV));
    return oldV == 0.0;}
  inline bool cond(const uintE &d) 
  { return Visited[d] == 0; }};

//Update function for backwards phase
struct BC_Back_F {
  fType* Dependencies;
  bool* Visited;
  BC_Back_F(fType* _Dependencies, bool* _Visited) 
  : Dependencies(_Dependencies), Visited(_Visited) {}
  inline bool update(const uintE &s, const uintE &d){ 
    fType oldV = Dependencies[d];
    Dependencies[d] += Dependencies[s];
    return oldV == 0.0;}
  inline bool updateAtomic(const uintE &s, const uintE &d){ 
    volatile fType oldV, newV;
    do {
      oldV = Dependencies[d];
      newV = oldV + Dependencies[s];
    } while(!CAS(&Dependencies[d],oldV,newV));
    return oldV == 0.0;}
  inline bool cond(const uintE &d) 
  { return Visited[d] == 0; }};

//vertex map function to mark visited vertices
struct BC_Vertex_F {
  bool* Visited;
  BC_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator() (const uintE &i) {
    Visited[i] = 1;
    return 1;}};

//vertex map function (used on backwards phase) to mark visited vertices
//and add to Dependencies score
struct BC_Back_Vertex_F {
  bool* Visited;
  fType* Dependencies, *inverseNumPaths;
  BC_Back_Vertex_F(bool* _Visited, fType* _Dependencies, fType* _inverseNumPaths) : 
    Visited(_Visited), Dependencies(_Dependencies), inverseNumPaths(_inverseNumPaths) {}
  inline bool operator() (const uintE &i) {
    Visited[i] = 1;
    Dependencies[i] += inverseNumPaths[i];
    return 1; }};

template <class vertex>
void BC(uintE start, graph<vertex> GA) {
  startTime();
  long n = GA.n, threshold = GA.m/20;

  fType* NumPaths = newA(fType,n);
  {parallel_for(long i=0;i<n;i++) NumPaths[i] = 0.0;}
  NumPaths[start] = 1.0;

  bool* Visited = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) Visited[i] = 0;}
  Visited[start] = 1;
  vertices Frontier(n,start);
 
  vector<vertices> Levels;
  Levels.push_back(Frontier);

  long round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){ //first phase
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    vertices output = edgeMap(GA, Frontier, BC_F(NumPaths,Visited),threshold,DENSE);
    vertexMap(output, BC_Vertex_F(Visited)); //mark visited
    Levels.push_back(output); //save frontier onto Levels
    Frontier = output;
  }

  fType* Dependencies = newA(fType,n);
  {parallel_for(long i=0;i<n;i++) Dependencies[i] = 0.0;}

  //invert numpaths
  fType* inverseNumPaths = NumPaths;
  {parallel_for(long i=0;i<n;i++) inverseNumPaths[i] = 1/inverseNumPaths[i];}

  Levels[round].del();
  //reuse Visited
  {parallel_for(long i=0;i<n;i++) Visited[i] = 0;}
  Frontier = Levels[round-1];
  vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));

  //tranpose graph
  GA.transpose();
 
  for(long r=round-2;r>=0;r--) { //backwards phase
    vertices output = edgeMap(GA, Frontier, 
			      BC_Back_F(Dependencies,Visited),threshold,DENSE/*_PARALLEL*/);
    output.del(); Frontier.del();
    Frontier = Levels[r]; //gets frontier from Levels array
    //vertex map to mark visited and update Dependencies scores
    vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));
  }
  
  Frontier.del();

  //Update dependencies scores
  parallel_for(long i=0;i<n;i++) {
    Dependencies[i]=(Dependencies[i]-inverseNumPaths[i])/inverseNumPaths[i];
  }
  free(inverseNumPaths);
  //for(intT i=0;i<n;i++) cout<<Dependencies[i]<<" ";cout<<endl;
  free(Visited);
  free(Dependencies);
  nextTime("BC");
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  long start = P.getOptionLongValue("-r",0);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric);
    BC((uintE)start,G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric);
    BC((uintE)start,G);
    G.del();
  }
}
