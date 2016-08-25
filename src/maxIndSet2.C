#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "graph.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

// **************************************************************
//    MAXIMAL INDEPENDENT SET
// **************************************************************

// vertices : the indices of vertices still being worked on
//   Flags[i] == 0 means still working on (in Remain)
//   Flags[i] == 1 means in the MIS
//   Flags[i] == 2 means at least one neighbor is in the MIS
void maxIndSetReadV1(seq<vindex> vertices, int n, vertex* G, int* Flags){
  int end = n;
  int wasted = 0;
  int ratio = 100;
  int maxR = 1 + n/ratio;
  int round = 0;
  vindex *hold = newA(vindex,maxR);
  bool *keep = newA(bool,maxR);
  char *tmpFlags = newA(char,maxR);
  vindex *vtx = vertices.S;

  while (end > 0) {
    //cout << "end=" << end << endl;
    round++;
    int size = min(maxR,end); // 1 + end/ratio;
    int start = end - size;

    cilk_for(int i=0;i<size;i++){ 
      vindex v = vtx[start+i];
      hold[i] = v;
      int d = G[v].degree;
      int j = 0;
      for (j ; j < d; j++){
	vindex ngh = G[v].Neighbors[j];
	int fl = Flags[ngh];
	if (fl == 1) { tmpFlags[i] = 2; break;}
	else if (fl == 0 && ngh >= v) { tmpFlags[i] = 0; break;}
      }
      if (j == d) { tmpFlags[i] = 1; }
    }

    cilk_for(int i=0;i<size;i++) {
      vindex v = vtx[start+i];
      if (tmpFlags[i] == 0) keep[i] = 1;
      else {
	Flags[v] = tmpFlags[i];
	keep[i] = 0;
      }
    }

    int nn = sequence::pack(hold,vtx+start,keep,size);
    end = start + nn;
    wasted += nn;
  }
  cout << "wasted = " << wasted << " rounds = " << round  << endl;
}

void maxIndSetRead(seq<vindex> vertices, int n, vertex* G, int* Flags){
  int end = n;
  int wasted = 0;
  int ratio = 100;
  int maxR = 1 + n/ratio;
  int round = 0;
  vindex *hold = newA(vindex,maxR);
  bool *keep = newA(bool,maxR);
  vindex *vtx = vertices.S;

  while (end > 0) {
    round++;
    int size = min(maxR,end); 
    int start = end - size;

    cilk_for(int i=0;i<size;i++){ 
      vindex v = vtx[start+i];
      hold[i] = v;
      keep[i] = 0;
      int d = G[v].degree;
      int j = 0;
      for (j ; j < d; j++){
	vindex ngh = G[v].Neighbors[j];
	int fl = Flags[ngh];
	if (fl == 1) { Flags[v] = 2; break;}
	else if (fl == 0 && ngh >= v) { keep[i] = 1; break;}
      }
      if (j == d) { Flags[v] = 1; }
    }

    int nn = sequence::pack(hold,vtx+start,keep,size);
    end = start + nn;
    wasted += nn;
  }
  free(keep); free(hold);
  cout << "wasted = " << wasted << " rounds = " << round  << endl;
}

void maxIndSetSerial(seq<vindex> vertices, int n, vertex* G, int* Flags){
  for(int i=0;i<n;i++){
    vindex v = vertices[i];
    Flags[v] = 1;
    for(int j=0;j<G[v].degree;j++){
      vindex ngh = G[v].Neighbors[j];
      if(Flags[ngh] == 1) {
	Flags[v] = 2;
	break;
      }
    }
  }
}

void maxIndSetNonDeterministic(seq<vindex> vertices, int n, vertex* G, int* Flags){
  int* V = newA(int,n);
  cilk_for(int i=0;i<n;i++) V[i]=INT_MAX;
  
  cilk_for(int i=0;i<n;i++){
    vindex v = vertices[i];
    while(1){
      //drop out if already in or out of MIS
      if(Flags[v]) break;
      int k = -1;
      //try to lock self and neighbors
      for(int j=-1;j<G[v].degree;j++){
	if(j == -1){ //acquire lock on self
	  if(utils::CAS(&V[v], INT_MAX, 0)) k++;
	  else break;
	} else {
	  vindex ngh = G[v].Neighbors[j];
	  //if ngh is not in MIS or we successfully 
	  //acquire lock, increment k
	  if(Flags[ngh] || utils::CAS(&V[ngh], INT_MAX, 0))
	    k++;
	  else break;
	}
      }   
      if(k == G[v].degree){ 
	//win on self and neighbors so fill flags
	Flags[v] = 1;
	for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j]; 
	  if(Flags[ngh] != 2) Flags[ngh] = 2;
	}
      } else { 
	//lose so reset V values up to point
	//where it lost
	for(int j = -1; j < k; j++){
	  if(j == -1) V[v] = INT_MAX;
	  else {
	    vindex ngh = G[v].Neighbors[j];
	    V[ngh] = INT_MAX;
	  }
	}
      }
    }
  }
  free(V);
}

struct stillSearching_FF {
  vertex *_G;
  int *_Flags;
  stillSearching_FF(vertex *G, int *Flags) : _G(G), _Flags(Flags) {}
  int operator() (vindex v) {
    if (_Flags[v] == 1) return 0;
    // if any neighbor is selected then drop out and set flag=2
    for (int i=0; i < _G[v].degree; i++) {
      vindex ngh = _G[v].Neighbors[i];
      if (_Flags[ngh] == 1) {_Flags[v] = 2; break;}
    }
    if (_Flags[v] == 0) return 1;
    else return 0;
  }
};



struct stillSearching {
  int *_Flags;
  stillSearching(int *Flags) : _Flags(Flags) {}
  int operator() (vindex v) {
    return _Flags[v] == 0;
  }
};

// vertices : the indices of vertices still being worked on
//   Flags[i] == 0 means still working on (in Remain)
//   Flags[i] == 1 means in the MIS
//   Flags[i] == 2 means at least one neighbor is in the MIS
void maxIndSetReadV2(seq<vindex> vertices, int n, vertex* G, int* Flags){
  int end = n;
  int ratio = 5;
  int maxR = 1 + n/ratio;

  int round = 0;

  while(vertices.size() > 0){ 
    round++;
    int size = min(maxR,end); // 1 + end/ratio;
    int start = end - size;
    cilk_for(int i=0;i<size;i++){ 
      vindex v = vertices[start+i];
      Flags[v] = 1;
      for(int j=0;j<G[v].degree;j++){
	vindex ngh = G[v].Neighbors[j];
	if(Flags[ngh] < 2 && ngh >= v){
	  Flags[v] = 0; break; }
      }
    }
    vertices = vertices.filter(stillSearching_FF(G,Flags));
    //maxIndSetRead(R,n,G,Flags); R.del();
    //int nn = sequence::filter(vertices,vertices,end,stillSearching_FF(G,Flags));
    //cilk_for (int i=0; i < R.sz; i++) vertices[i] = R[i];
    end = vertices.size();
  }

}

void maxIndSetWrite(seq<vindex> vertices, int n, vertex* G, int* Flags){

  int* V = newA(int,n);
  cilk_for(int i=0;i<n;i++) V[i]=INT_MAX;

  int round = 0;
  while(vertices.size() > 0){ 
    round++;
    cilk_for(int i=0;i<vertices.size();i++){ 
      vindex v = vertices[i];
      utils::writeMin(&V[v],v);
      for(int j=0;j<G[v].degree;j++){
	vindex ngh = G[v].Neighbors[j];
	
	if(Flags[ngh] == 1) { Flags[v] = 2; break; }
	utils::writeMin(&V[ngh],v);
      }
    }
 
    cilk_for(int i=0;i<vertices.size();i++){
      vindex v = vertices[i];
      //equal to self and alive
      if(Flags[v] < 2 && V[v] == v){
	Flags[v] = 1;
	for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j];
	  if(Flags[ngh] < 2) Flags[ngh] = 2;
	}

      } else {
	if(V[v] == v) V[v] = INT_MAX;
	for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j];
	  //reset neighbor
	  if(V[ngh] == v){
	    V[ngh] = INT_MAX;
	  }
	}
      }
    }
    //reset values
    //cilk_for(int i=0;i<n;i++) V[i] = INT_MAX;
    //for(int i=0;i<vertices.size();i++)cout<<Flags[vertices[i]]<<" ";cout<<endl;
    vertices = vertices.filter(stillSearching(Flags));

  }
  

}

// Checks if valid maximal independent set
void checkMaximalIndependentSet(graph G, int* Flags) {
  int n = G.n;
  vertex* V = G.V;
  cilk_for (int i=0; i < n; i++) {
    int nflag;
    for (int j=0; j < V[i].degree; j++) {
      vindex ngh = V[i].Neighbors[j];
      if (Flags[ngh] == 1)
	if (Flags[i] == 1) {
	  cout << "checkMaximalIndependentSet: bad edge " 
	       << i << "," << ngh << endl;
	  abort();
	} else nflag = 1;
    }
    if ((Flags[i] != 1) && (nflag != 1)) {
      cout << "checkMaximalIndependentSet: bad vertex " << i << endl;
      abort();
    }
  }
}

int* maxIndependentSet(graph G, int seed) {
  int n = G.n;
  int round = seed;
  seq<vindex> vertices = seq<vindex>(n, utils::identityF<int>());
  seq<int> Flags = seq<int>(n, utils::zeroF<int>());
  //maxIndSetSerial(vertices,vertices.size(),G.V,Flags.S);
  //maxIndSetNonDeterministic(vertices,vertices.size(),G.V,Flags.S);
  maxIndSetRead(vertices,vertices.size(),G.V,Flags.S);
  //maxIndSetWrite(Remain,Remain.size(),G.V,Flags.S);
  vertices.del();
  return Flags.S;
}
