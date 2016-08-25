#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "sequence.h"
#include "graph.h"
#include "gettime.h"
#include "cilk.h"
#include "manis.h"
#include "limits.h"
using namespace std;

struct notNeg { bool operator () (int i) {return (i >= 0);}};

void manisLoop(set* sets, int numSets, int numElts,
	       seq<int> setNum, seq<int> eltNums,
	       bool* flags){
  float eps = 0.05;
  float c = 1-4*eps; //constant for determining covering threshold
  float b = 1-eps; //constant for determining "too few" neighbors
  //int setsRemaining = numSets;
  //int eltsRemaining = numElts;

  int* reserve = newA(int,numElts);
  cilk_for(int i=0;i<numElts;i++) reserve[i] = INT_MAX;

  int *setNums = setNum.S;

  int end = numSets;
  int wasted = 0;
  int ratio = 100;
  int maxR = 1 + numSets/ratio;
  int round = 0;
  int *hold = newA(int,maxR); //temp array for pack
  bool *keep = newA(bool,maxR); //0 if win, 1 if lose

  int* fl = newA(int,maxR*numElts);
  int* counts = newA(int,maxR);


  //while(setsRemaining > 0){
    //cout<<setsRemaining<< " " <<endl;
  while(end > 0){
    round++;
    int size = min(maxR,end);
    int start = end - size;

    cilk_for(int i=0;i<size*numElts;i++) fl[i] = 0;

    //reserve
    cilk_for(int i=0;i<size;i++){
      int s = setNums[start+i];
      cilk_for(int j=0;j<sets[s].size;j++){
	int ngh = sets[s].Elts[j];
	utils::writeMin(&reserve[ngh],s);
      }
    }
    //cout<<"reserved\n";

    //for(int i=0;i<numElts;i++)cout<<reserve[i]<<" ";cout<<endl;

    //count winners
    cilk_for(int i=0;i<size;i++){
      int s = setNums[start+i];
      hold[i] = s; 
      keep[i] = 1;
      cilk_for(int j=0;j<sets[s].size;j++){
	int ngh = sets[s].Elts[j];
	if(reserve[ngh] == s) {
	  fl[i*numElts+ngh] = 1;
	  reserve[ngh] = INT_MAX;
	}
      }
      counts[i] = sequence::plusScan(fl+i*numElts,fl+i*numElts,numElts);
    }
    //cout<<"counted winners\n";

    //for(int i=0;i<setsRemaining;i++)cout<<counts[i]<<" ";cout<<endl;

    //see if enough elts covered.
    //if so then choose it and
    //remove covered elements
    cilk_for(int i=0;i<size;i++){
      int s = setNums[start+i];
      if(counts[i] > c*sets[s].size){
	flags[s] = 1;
	keep[i] = 0;
	cilk_for(int j=0;j<sets[s].size;j++){
	  int ngh = sets[s].Elts[j];
	  eltNums[ngh] = -1;
	}
	setNums[start+i] = -1;
      }
    }
    //cout<<"checked\n";
    //for(int i=0;i<setsRemaining;i++)cout<<setNums[i]<<" ";cout<<endl;
    //for(int i=0;i<numSets;i++)cout<<flags[i]<<" ";cout<<endl;

    //remove sets with too few remaining neighbors
    cilk_for(int i=0;i<size*numElts;i++) fl[i] = 0;
    cilk_for(int i=0;i<size;i++){
      int s = setNums[start+i];
      if(s != -1){ //compute number of neighbors still alive
	cilk_for(int j=0;j<sets[s].size;j++){
	  int ngh = sets[s].Elts[j];
	  if(eltNums[ngh] != -1) fl[i*numElts+ngh] = 1;
	  else sets[s].Elts[j] = -1; //mark dead neighbor
	}
	int count = sequence::plusScan(fl+i*numElts,fl+i*numElts,numElts);
	if((float)count < b*sets[s].size) {
	  //setNums[start+i] = -1; //remove set
	  keep[i] = 0;
	}
	else { //pack neighbors
	  int d = sequence::filter(sets[s].Elts,sets[s].Elts,
				   sets[s].size,notNeg());
	  sets[s].size = d;
	}
      }
    }
    //for(int i=0;i<setsRemaining;i++)cout<<setNums[i]<<" ";cout<<endl;
    //cout<<"removed\n";
    //for(int i=0;i<numElts;i++)cout<<eltNums[i]<<" ";cout<<endl;

    int nn = sequence::pack(hold,setNums+start,keep,size);

    //setNums = setNums.filter(notNeg());
    //cout<<"filtered\n";

    //for(int i=0;i<setNums.sz;i++)cout<<setNums[i]<<" ";cout<<endl;
    //for(int i=0;i<eltNums.sz;i++)cout<<eltNums[i]<<" ";cout<<endl;
    if(size == nn) break;
    //setsRemaining = setNums.size();
    end = start+nn;
    wasted += nn;
   
  }
  free(reserve);
  free(fl);
  free(counts);
}

inline void initFlags(bool* flags, int numSets){
  cilk_for(int i=0;i<numSets;i++) flags[i] = 0;
}

void manis(set* sets, int numSets, int numElts) {

  //flags[i] is 1 if set i is chosen
  bool* flags = newA(bool,numSets);
  initFlags(flags,numSets);

  seq<int> setNums = seq<int>(numSets,utils::identityF<int>());
  seq<int> eltNums = seq<int>(numElts,utils::identityF<int>());
  manisLoop(sets,numSets,numElts,setNums,eltNums,flags);

  setNums.del();
  eltNums.del();

  sequence::pack(sets,sets,flags,numSets);

  free(flags);

}
