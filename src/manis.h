#ifndef __MANIS_INCLUDED__
#define __MANIS_INCLUDED__

#include <cstdlib>
#include "cilk.h"
#include <algorithm>
#include "seq.h"
#include <string.h>
#include "hash.h"
#include "utils.h"

struct set {
  int* Elts;
  int size;
  void del() {free(Elts);}
  set(int* E, int s) : Elts(E), size(s) {}
};


inline void assignElts(set* sets, int numSets, int numEdges, int numElts){
 
  ulong numChoices = (ulong)numSets*numElts;
  if(numChoices < numEdges || numChoices > ((ulong)1<<32)) {
    cout<<"Too many edges\n";
    abort();
  }
  bool* flags = newA(bool,numChoices);
  int density = 1+numChoices/numEdges;
  cilk_for(int i=0;i<numChoices;i++){
    if(utils::hash(i)%density == 0) {flags[i] = 1;}
    else flags[i] = 0;
  }

  int* E = newA(int,numChoices);

  cilk_for(int i=0;i<numSets;i++){
    int offset = i*numElts;
    int* Elts = E+offset;
    cilk_for(int j=0;j<numElts;j++){
      if(flags[offset+j]) Elts[j] = j;
      else Elts[j] = 0;
    }
    int size = sequence::pack(Elts,Elts,flags+offset,numElts);
    sets[i].size = size;
    sets[i].Elts = Elts;
  }
  cout<<"b\n";
  free(flags);
}

//Random bipartite graph 
static set* edgeRandomBipartite(int numSets, int numEdges, int numElts){

  set* sets = newA(set,numSets);
  assignElts(sets,numSets,numEdges,numElts);
  
  return sets;  
}

#endif
