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

#define Kminus1 5
#define L6 0x41041041041041
#define H6 0x820820820820820
#define H6_comp 0xf7df7df7df7df7df

#define xLessY(_x, _y)					\
  (((((_x | H6) - (_y & H6_comp)) | (_x ^ _y)) ^ (_x | ~_y)) & H6);

#define xLessEqualY(_x, _y)					\
  (((((_y | H6) - (_x & H6_comp)) | (_x ^ _y)) ^ (_x & ~_y)) & H6);

#define mask(_xLessY)				\
    ((((_xLessY >> Kminus1) | H6) - L6) | H6) ^ _xLessY;

#define broadwordMax(_x,_y,_m)			\
    (_x & _m) | (_y & ~_m);

//atomically writeMax into location a
inline bool writeBroadwordMax(long *a, long b) {
  long c; bool r=0; long d, e, m;
  do {c = *a; d = xLessEqualY(b,c);
    if(d != H6) { //otherwise all values we are trying to write are smaller 
      m = mask(d);
      e = broadwordMax(b,c,m);
      r = CAS(a,c,e);
    } else break;}
    while (!r);
  return r;
}

//Update function does a broadword max on log-log counters
struct Radii_F {
  intE round;
  intE* radii;
  long* VisitedArray, *NextVisitedArray;
  long length;
  Radii_F(long _length, long* _Visited, long* _NextVisited, 
	       intE* _radii, intE _round) : 
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool update(const uintE &s, const uintE &d){
    bool changed = 0;
    for(long i=0;i<length;i++) {
      long nv = NextVisitedArray[d*length+i], v = VisitedArray[s*length+i];
      long a = xLessEqualY(v,nv);
      if(a != H6) {
	long b = mask(a);
	long c = broadwordMax(v,nv,b);
	NextVisitedArray[d*length+i] = c;
	if(radii[d] != round) { radii[d] = round; changed = 1; }
      }
    }
    return changed;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    bool changed = 0; 
    for(long i=0; i<length;i++) {
      if(writeBroadwordMax(&NextVisitedArray[d*length+i],VisitedArray[s*length+i])) {
	intE oldRadii = radii[d];
	if(radii[d] != round) if(CAS(&radii[d],oldRadii,round)) changed = 1;
      }
    }
    return changed;
  }
  inline bool cond(const uintE &i) { return cond_true(i);}};

//function passed to vertex map to sync NextVisited and Visited
struct Radii_Vertex_F {
  long* VisitedArray, *NextVisitedArray;
  long length;
  Radii_Vertex_F(long _length, long* _Visited, long* _NextVisited) :
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited) {}
  inline bool operator() (const uintE &i) {
    for(long j=i*length;j<(i+1)*length;j++)
      NextVisitedArray[j] = VisitedArray[j];
    return 1;
  }
};

template <class vertex>
intE* Radii(graph<vertex> GA, long length, uint seed) {
  startTime();
  long n = GA.n;

  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);
  intE* radii = newA(intE,n);

  parallel_for(ulong i=0;i<n*length;i++)  { 
    //initialize log-log counters (10 registers per counter)
    ulong counter = 0;
    for(ulong j=0;j<10;j++) {
      ulong rand = hash(i*10+j+(ulong)seed*10);
      ulong rightMostBit  = (rand == 0) ? 0 : log2(rand&-rand);
      counter |= (rightMostBit << (6*j));
    }
    VisitedArray[i] = counter;
  }
  // //test
  // long x, y;
  // x = 2;
  // y = 10;
  // x |= ((long)16 << 6);
  // y |= ((long)44 << 6);
  // x |= ((long)39 << 12);
  // y |= ((long)18 << 12);
  // x |= ((long)30 << 18);
  // y |= ((long)31 << 18);
  // x |= ((long)3 << 24);
  // y |= ((long)6 << 24);
  // x |= ((long)60 << 30);
  // y |= ((long)1 << 30);
  // x |= ((long)6 << 36);
  // y |= ((long)8 << 36);
  // x |= ((long)3 << 42);
  // y |= ((long)4 << 42);
  // x |= ((long)16 << 48);
  // y |= ((long)31 << 48);
  // x |= ((long)55 << 54);
  // y |= ((long)15 << 54);

  // long a = xLessY(x,y);
  // long aa = xLessEqualY(x,y);

  // long b = mask(aa);
  // long c = broadwordMax(x,y,b);
  // for(int i=0;i<10;i++) {
  //   cout << ((a >> i*6) & 0x3f) << " " ;
  // } cout << endl;
  // for(int i=0;i<10;i++) {
  //   cout << ((aa >> i*6) & 0x3f) << " " ;
  // } cout << endl;
  // for(int i=0;i<10;i++) {
  //   cout << ((b >> i*6) & 0x3f) << " " ;
  // } cout << endl;

  // for(int i=0;i<10;i++) {
  //   cout << ((c >> i*6) & 0x3f) << " " ;
  // }
  // cout << endl;
  //exit(0);

  {parallel_for(long i=0;i<n;i++) {
      radii[i] = 0;
    }}
  
  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;} 
  vertices Frontier(n,n,frontier); //initial frontier contains all vertices

  intE round = 0;
  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl<<flush;
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
  uint seed = P.getOptionIntValue("-seed",0);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric);
    intE* R = Radii(G,length,seed*G.n);
    if(oFile != NULL) {
      ofstream file (oFile, ios::out | ios::binary);
      stringstream ss;
      for(long i=0;i<G.n;i++) ss << R[i] << endl;
      file << ss.str();
      file.close();
    }
    G.del();
    free(R);
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric);
    Radii(G,length,seed*G.n);
    G.del();
  }
}
