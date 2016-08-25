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
#include "blockRadixSort.h"
#include "CCBFS.h"
using namespace std;

timer t0,t1,t2,t3,t4,t5,t6,t7,t8;

typedef pair<uintE,uintE> intPair;

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

template <class ET, class intT>
struct getVisited {
  ET* A;
  getVisited(ET* AA) : A(AA) {}
  ET operator() (intT i) {return A[i] != UINT_E_MAX;}
};

template <class vertex>
struct getDegree {
  vertex* V;
  getDegree(vertex* VV) : V(VV) {}
  intE operator() (intE i) {return V[i].getOutDegree();}
};

template <class ET, class intT>
struct getNonzero {
  ET* A;
  getNonzero(ET* AA) : A(AA) {}
  ET operator() (intT i) {return (A[i] > 0);}
};

struct maxFirstF { intPair operator() (const intPair& a, const intPair& b) 
  const {return (a.first>b.first) ? a : b;}};


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
      NextVisitedArray[j] = VisitedArray[j];
    return 1;
  }
};

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("CC preprocess");
  t2.reportTotal("CC with BFS");
  t3.reportTotal("CC with label propagation");
  t8.reportTotal("sort by component ID and compute sizes");
  t4.reportTotal("Radii phase 1");
  t5.reportTotal("sort by radii");
  t6.reportTotal("Radii phase 2");
}

template <class vertex>
uintE* Radii(graph<vertex> GA, long length, uintT seed) {
  startTime();
  t0.start();
  long n = GA.n;

  uintE* radii = newA(uintE,n);
  uintE* radii2 = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) {
      radii[i] = radii2[i] = 0;
    }}

  t0.stop();
  t1.start();
  
  intE* Labels = newA(intE,n);
  parallel_for(long i=0;i<n;i++) {
    if(GA.V[i].getOutDegree() == 0) Labels[i] = -i-1; //singletons
    else Labels[i] = INT_E_MAX;
  }

  // long numVisited = sequence::reduce<uintE>((intE)0,(intE)n,addF<intE>(),
  // 					    getVisited<intE,intE>(Labels));
  t1.stop();
  t2.start();

  //get max degree vertex
  uintE maxV = sequence::reduce<uintE>((intE)0,(intE)n,maxF<intE>(),getDegree<vertex>(GA.V));

  //visit large component with BFS
  CCBFS(maxV,GA,Labels);

  // for(long i=0;i<n;i++) {
  //   if(numVisited > n/2) break;
  //   if(Labels[i] == INT_E_MAX)
  //     numVisited += CCBFS(i,GA,Labels);
  // }
  t2.stop();
  t3.start();
  //visit small components with label propagation
  Components(GA, Labels);
  t3.stop();

  t8.start();
  //sort by component ID
  intPair* CCpairs = newA(intPair,n);
  parallel_for(long i=0;i<n;i++)
    if(Labels[i] < 0)
      CCpairs[i] = make_pair(-Labels[i]-1,i);
    else CCpairs[i] = make_pair(Labels[i],i);

  free(Labels);

  //cout<<"CCpairs: ";for(int i=0;i<n;i++)cout<<"("<<CCpairs[i].first<<"," <<CCpairs[i].second<<") ";cout<<endl;

  //to do: try packing IDs into tight range
  intSort::iSort(CCpairs, n, n+1,
		 firstF<uintE,uintE>());

  uintE* changes = newA(uintE,n);
  changes[0] = 0;
  parallel_for(long i=1;i<n;i++) 
    changes[i] = (CCpairs[i].first != CCpairs[i-1].first) ? i : UINT_E_MAX;

  //cout<<"changes: ";for(int i=0;i<n;i++)cout<<changes[i]<<" ";cout<<endl;

  uintE* CCoffsets = newA(uintE,n);
  uintE numCC = sequence::filter(changes, CCoffsets, n, nonMaxF());
  CCoffsets[numCC] = n;
  free(changes);
  //cout << "numCC = " << numCC << endl;

  //cout <<"CCoffsets: ";for(int i=0;i<numCC;i++) cout << CCoffsets[i] << " " ; cout << endl;

  t8.stop();
  t0.start();

  length = min((n+63)/64,length);
  //cout << numCC << " " << length << endl;
  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);
  
  int* flags = newA(int,n);
  parallel_for(long i=0;i<n;i++) flags[i] = 0;

  uintE* starts = newA(uintE,n);
  
  intPair* pairs = newA(intPair,n);
  t0.stop();

  for(long k = 0; k < numCC; k++) {
    t4.start();
    uintE o = CCoffsets[k];
    uintE CCsize = CCoffsets[k+1] - o;
    //cout << k << " " << o << " " << CCsize << endl;
    //if(CCsize == 1) numLongs[k] = 0; //singletons have radii of 0
    if(CCsize == 2) { //size 2 CC's have radii of 1
      radii[CCpairs[o].second] = radii[CCpairs[o+1].second] = 1;
      //numLongs[k] = 0;
      t4.stop();
    } else if(CCsize > 1) {
      //do main computation
      long myLength = min(length,((long)CCsize+63)/64);

      //initialize bit vectors for component vertices
      parallel_for(long i=0;i<CCsize;i++) {
	uintT v = CCpairs[o+i].second;
	parallel_for(long j=0;j<myLength;j++)
	  VisitedArray[v*myLength+j] = NextVisitedArray[v*myLength+j] = 0;
      }
      //cout << myLength << endl;
      long sampleSize = min((long)CCsize,(long)64*myLength);
      //cout << sampleSize << endl;

      //uintE* starts = newA(uintE,sampleSize);
      uintE* starts2 = newA(uintE,sampleSize);

      //pick random vertices (could have duplicates)
      parallel_for(ulong i=0;i<sampleSize;i++) {
	uintT index = hash(i+seed) % CCsize;
	if(flags[index] == 0 && CAS(&flags[index],0,(int)i)) {
	  starts[i] = CCpairs[o+index].second;
	  VisitedArray[CCpairs[o+index].second*myLength + i/64] = (long) 1<<(i%64);
	} else starts[i] = UINT_E_MAX;
      }

      //remove duplicates
      uintE numUnique = sequence::filter(starts,starts2,sampleSize,nonMaxF());
      //free(starts);

      //reset flags
      parallel_for(ulong i=0;i<sampleSize;i++) {
	uintT index = hash(i+seed) % CCsize;
	if(flags[index] == i) flags[index] = 0;
      }

      //first round
      vertices Frontier(n,numUnique,starts2); //initial frontier

      uintE round = 0;
      while(!Frontier.isEmpty()){
	round++;
	vertexMap(Frontier, Radii_Vertex_F(myLength,VisitedArray,NextVisitedArray));
	vertices output = 
	  edgeMap(GA, Frontier, 
		  Radii_F(myLength,VisitedArray,NextVisitedArray,radii,round),
		  GA.m/20);
	swap(NextVisitedArray,VisitedArray);
	Frontier.del();
	Frontier = output;
      }
      t4.stop();
      //second round if size of CC > 64
      if(CCsize > 1024) {
	//sort by radii
	t5.start();
	parallel_for(long i=0;i<CCsize;i++) {
	  pairs[i] = make_pair(radii[CCpairs[o+i].second],CCpairs[o+i].second);
	}

	intPair maxR = sequence::reduce(pairs,CCsize,maxFirstF());

	intSort::iSort(pairs, CCsize, 1+maxR.first, firstF<uintE,uintE>());
	//cout << maxR.first << endl;
	
	t5.stop();

	t6.start();

	//reset bit vectors for component vertices
	parallel_for(long i=0;i<CCsize;i++) {
	  uintT v = CCpairs[o+i].second;
	  parallel_for(long j=0;j<myLength;j++)
	    VisitedArray[v*myLength+j] = NextVisitedArray[v*myLength+j] = 0;
	}

	starts2 = newA(uintE,sampleSize);
	//pick starting points with highest radii ("fringe" vertices)
	parallel_for(long i=0;i<sampleSize;i++) {
	  intE v = pairs[CCsize-i-1].second;
	  starts2[i] = v;
	  VisitedArray[v*myLength + i/64] = (long) 1<<(i%64);
	}

	vertices Frontier2(n,sampleSize,starts2); //initial frontier

	round = 0;
	while(!Frontier2.isEmpty()){
	  round++;
	  //cout<<"Round "<<round<<" "<<Frontier2.numNonzeros()<<endl<<flush;
	  vertexMap(Frontier2, Radii_Vertex_F(myLength,VisitedArray,NextVisitedArray));
	  vertices output = 
	    edgeMap(GA, Frontier2, 
		    Radii_F(myLength,VisitedArray,NextVisitedArray,radii2,round),
		    GA.m/20);
	  swap(NextVisitedArray,VisitedArray);
	  Frontier2.del();
	  Frontier2 = output;
	}
	Frontier2.del();
	parallel_for(long i=0;i<n;i++) radii[i] = max(radii[i],radii2[i]);

	t6.stop();
      }
    }
  }
  t0.start();
  free(flags); free(pairs); free(VisitedArray); free(NextVisitedArray); free(radii2);
  free(CCoffsets); free(CCpairs);
  t0.stop();
  reportAll();
  nextTime("All Radii");
  return radii;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] [-l <length>] [-o <oFile>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  bool symmetric = P.getOptionValue("-s");
  long length = P.getOptionLongValue("-l",1);
  uintT seed = P.getOptionLongValue("-seed",0);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric);
    uintE* radii = Radii(G,length, seed*G.n);
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
    Radii(G,length, seed*G.n);
    G.del();
  }
}
