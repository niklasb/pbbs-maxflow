#include <iostream>
#include "limits.h"
#include "seq.h"
#include "gettime.h"
#include "cilk.h"
#include "graph.h"
#include "utils.h"
#include "hash.h"
#include "quickSort.h"
using namespace std;

typedef int eindex;

inline int hashEdge(eindex i) {
  return int ((((unsigned int) -1) >> 1) & utils::hash(i));
}

void maxMatch(edge* E, vindex* vertices, int m) {

  int end = m;
  int ratio = 100;
  int maxR = 1 + m/ratio;
  int wasted = 0;
  int round = 0;
  bool* f = newA(bool,maxR);
  int* ID = newA(int,maxR);
  edge* hold = newA(edge,maxR);

  while (end > 0) {
    round++;
    int size = min(maxR,end); // 1 + end/ratio;
    int start = end - size;

    cilk_for(int i = 0; i < size; i++){
      vindex v = E[i+start].v;
      vindex u = E[i+start].u;
      if(vertices[v] >= 0 && vertices[u] >= 0){
	//int h = i; 
	//int h = hashEdge(i);
	utils::writeMin(&vertices[v],i);
	utils::writeMin(&vertices[u],i);
	f[i] = 1;
      } else {
	f[i] = 0; //edge drops out 
      }
    }

    //check if won
    cilk_for(int i=0; i < size; i++) {
      if (f[i] == 1) {
	vindex v = E[i+start].v;
	vindex u = E[i+start].u;
	hold[i] = E[i+start];
	//int h = i; 
	//int h = hashEdge(i);
	if (vertices[v] == i) {
	  if (vertices[u] == i) {
	    vertices[v] = -u-1;
	    vertices[u] = -v-1;
	    f[i] = 0; //edge is in matching
	  } else vertices[v] = INT_MAX;
	} else if (vertices[u] == i)
	  vertices[u] = INT_MAX;
      }
    }

    //filter out edges that are dropping out
    int nn = sequence::packIndex(ID,f,size);
    cilk_for (int i=0; i < nn; i++) E[start+i] = hold[ID[i]];

    end = end - size + nn;
    wasted += nn;
    
  }
  cout << "wasted = " << wasted << " rounds = " << round  << endl;
}

void maxMatchSerial(edge* E, vindex* vertices, int m) {
  for (int i = 0; i < m; i++) {
    vindex v = E[i].v;
    vindex u = E[i].u;
    if(vertices[v] >= 0 && vertices[u] >= 0){
      vertices[v] = -u-1;
      vertices[u] = -v-1;
    }
  }
}

void maxMatchNonDeterministic(edge* E,  vindex* vertices, int m) {
  cilk_for (int i = 0; i < m; i++) {
    vindex v = E[i].v;
    vindex u = E[i].u;
    int j =0;
    while (1) {
      if (j++ > 20) abort();
      if (vertices[v] < 0 || vertices[u] < 0) break;
      if (utils::CAS(&vertices[v], INT_MAX, 0)) {
	if (utils::CAS(&vertices[u], INT_MAX, 0)) {
	  vertices[v] = -u-1;
	  vertices[u] = -v-1;
	  break;
	} else vertices[v] = INT_MAX;
      }
    }
  }
}


void initVertices(vindex* vertices, int n){
  cilk_for(int i=0;i<n;i++)
    vertices[i] = INT_MAX;
}

// Finds a maximal matching of the graph
// Returns edgeflags, an array of flags, where edgeflags[i] = 1
// iff edge EA.E[i] is in the maximal matching
vindex* maximalMatching(edgeArray EA, int n) {
  int m = EA.nonZeros;

  vindex* vertices = newA(vindex,n);
  initVertices(vertices,n);

  maxMatch(EA.E, vertices, m);
  //maxMatchSerial(EA.E, vertices, m);
  //maxMatchNonDeterministic(EA.E, vertices, m);

  return vertices;
}  


struct neg { bool operator () (int i) {return (i < 0);}};

void checkMaximalMatching(vindex* vertices, int n, edgeArray EA){

  //loop over edges to check if any could be added
  cilk_for(int i=0;i<EA.nonZeros;i++){
    eindex ve = vertices[EA.E[i].v];
    eindex ue = vertices[EA.E[i].u];
    if(ve >= 0 && ue >= 0){
      cout<<"matching not maximal: could add edge "<<i<<endl;
      abort();
    }
  }

  //check if a vertex was "hooked" multiple times
  //if so, there are more than one edge in the matching
  //incident on this vertex, hence it is not a
  //valid matching
  vindex* temp = newA(vindex,n);
  
  int m = sequence::filter(vertices,temp,n,neg());
  
  cilk_for(int i=0;i<m;i++)
    temp[i] *= -1; //to get remove duplicates to work

  seq<int> r = removeDuplicates(seq<int>(temp,m));
  
  if(m != r.sz){
    cout<<"matching not valid"<<endl;
    abort();
  }
  free(temp); r.del();
}
