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
#include "gettime.h"
#include "parseCommandLine.h"
#include <sstream>
using namespace std;

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

//Update function does a bitwise-or
struct Radii_F {
  uintE round;
  uintE* radii;
  long* VisitedArray, *NextVisitedArray;
  const long length;
  Radii_F(const long _length, long* _Visited, long* _NextVisited, 
	       uintE* _radii, uintE _round) : 
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool update(const uintE &s, const uintE &d){
    bool changed = 0;
    for(long i=0;i<length;i++) {
      long toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	NextVisitedArray[d*length+i] |= toWrite;
	if(radii[d] < round) { radii[d] = round; changed = 1; }
      }
    }
    return changed;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    bool changed = 0; 
    for(long i=0; i<length;i++) {
      long toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	writeOr(&NextVisitedArray[d*length+i],toWrite);
	uintE oldRadii = radii[d];
	if(radii[d] < round) if(CAS(&radii[d],oldRadii,round)) changed = 1;
      }
    }
    return changed;
  }
  inline bool cond(const uintE &i) {return cond_true(i);}};

//function passed to vertex map to sync NextVisited and Visited
struct Radii_Vertex_F {
  long* VisitedArray, *NextVisitedArray;
  const long length;
  Radii_Vertex_F(const long _length, long* _Visited, long* _NextVisited) :
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited) {}
  inline bool operator() (const uintE &i) {
    for(long j=i*length;j<(i+1)*length;j++)
      NextVisitedArray[j] |= VisitedArray[j];
    return 1;
  }
};

template <class vertex>
uintE* Radii(graph<vertex> GA, long length) {
  startTime();
  long n = GA.n;

  length = min((n+63)/64,length);
  
  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);
  uintE* radii = newA(uintE,n);

  parallel_for(long i=0;i<n*length;i++) 
    VisitedArray[i] = NextVisitedArray[i] = 0;

  {parallel_for(long i=0;i<n;i++) {
      radii[i] = 0;
    }}
  long sampleSize = min(n,(long)64*length);
  bool* starts = newA(bool,n);
  parallel_for(long i=0;i<n;i++) starts[i] = 0;

  //pick random vertices (could have duplicates)
  parallel_for(ulong i=0;i<sampleSize;i++) {
    uintT v = hash(i) % n;
    starts[v] = 1; 
    VisitedArray[v*length + i/64] = (long) 1<<(i%64);
  }

  vertices Frontier(n,starts); //initial frontier

  uintE round = 0;
  while(!Frontier.isEmpty()){
    round++;
    cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl<<flush;
    vertexMap(Frontier, Radii_Vertex_F(length,VisitedArray,NextVisitedArray));
    vertices output = 
      edgeMap(GA, Frontier, 
	      Radii_F(length,VisitedArray,NextVisitedArray,radii,round),
	      GA.m/20);
    swap(NextVisitedArray,VisitedArray);
    Frontier.del();
    Frontier = output;
  }
  Frontier.del();

  free(VisitedArray); free(NextVisitedArray); 
  
  nextTime("All Radii");
  return radii;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] [-l <length>] [-o <oFile>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  bool symmetric = P.getOptionValue("-s");
  long length = P.getOptionLongValue("-l",1);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric);
    uintE* radii = Radii(G,length);
    if(oFile != NULL) {
      ofstream file (oFile, ios::out | ios::binary);
      stringstream ss;
      for(long i=0;i<G.n;i++) ss << radii[i] << endl;
      file << ss.str();
      file.close();
    }
    G.del();
    free(radii);
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric);
    Radii(G,length);
    G.del();
  }
}
