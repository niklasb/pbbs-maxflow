#include <iostream>
#include "gettime.h"
#include "ANSV.h"
#include "blockRadixSort.h"
#include "sequence.h"
#include "deterministicHash.h"
#include "suffixTree.h"

using namespace std;

void radixSortPair(pair<intT,uintT> *A, long n, long m) {
  intSort::iSort(A,n,m,utils::firstF<intT,uintT>());
}

struct notNeg { bool operator () (intT i) {return (i >= 0);}};

//Note: deletes SA and LCP
suffixTree suffixArrayToTree(uintT* SA, uintT* LCP, long n, uintT* s){
  startTime();
  //compute all nearest smaller values
  // int* leftNeighbors = new int[n];
  // int* rightNeighbors = new int[n];
  // ComputeANSV(LCP, n, leftNeighbors, rightNeighbors);

  intT* leftNeighbors = new intT[n];
  intT* rightNeighbors = new intT[n];
  ComputeANSV((intT*)LCP, n, leftNeighbors, rightNeighbors);

  // ANSV ANSV(LCP,n);
  // int* leftNeighbors = ANSV.getLeftNeighbors();
  // int* rightNeighbors = ANSV.getRightNeighbors();
  nextTime("ANSV");
  
  pair<intT,uintT>* parentPlusDirectionPairs = new pair<intT,uintT>[n];

  intT* parents = new intT[n];
  //uint* direction = new uint[n]; 
  //direction: 0 indicates parent is coming from left, 
  //n+1 indicates parent is coming from right, -1 indicates no parent
  //compute the parent and direction of parent for each suffix
  cilk_for(intT i=0;i<n;i++){
    uintT direction;
    if(leftNeighbors[i] != -1 && rightNeighbors[i] != -1){
      parents[i] = (LCP[leftNeighbors[i]] >= LCP[rightNeighbors[i]])
	? leftNeighbors[i] : rightNeighbors[i]; //take left neighbor if tie
      direction = (LCP[leftNeighbors[i]] >= LCP[rightNeighbors[i]]) 
	? 0 : n+1;
    }
    else if(leftNeighbors[i] == -1 && rightNeighbors[i] == -1) 
      { parents[i] = -1; direction = 0; } //no parent
    else if(leftNeighbors[i] == -1) 
      { parents[i] = rightNeighbors[i]; direction = n+1; } //right parent
    else
      { parents[i] = leftNeighbors[i]; direction = 0; } //left parent
    parentPlusDirectionPairs[i].second = i; 
    parentPlusDirectionPairs[i].first = 1 + parents[i] + direction;
  }

  //pair<int,uint>* parentPlusDirectionPairs = new pair<int,uint>[n];

  // cilk_for(int i=0;i<n;i++){
  //   parentPlusDirectionPairs[i].first = 1 + parents[i] + direction[i];
  //   parentPlusDirectionPairs[i].second = i;
  // }  

  delete [] leftNeighbors;
  delete [] rightNeighbors;

  nextTime("parents");

  radixSortPair(parentPlusDirectionPairs,n,2*n+2);
  nextTime("sort");

  //delete [] direction;

  intT *offsets = new intT[n];
  intT *oout = new intT[n+1];
  cilk_for (intT i=1; i < n; i++) {
    offsets[i] = parentPlusDirectionPairs[i].first 
      == parentPlusDirectionPairs[i-1].first ? -1 : i;
  }
  offsets[0] = 0;
  intT nm = sequence::filter(offsets,oout,n,notNeg());
  oout[nm] = n;
  //nextTime("time for collect");

  delete [] offsets;

  //use this array to check which supernode a node belongs to
  uintT* superNodeID = new uintT[n];
  cilk_for (intT i=0; i<nm; i++) {
      intT l = oout[i+1]-oout[i];
      cilk_for (intT j=0; j < l; j++) {
	superNodeID[parentPlusDirectionPairs[j+oout[i]].second] = i;
      }
  }


  //leaves
  uintT* leafSuperNodeID = new uintT[n];
  uintT* leafDepth = new uintT[n];
  leafSuperNodeID[0] = superNodeID[0];
  leafDepth[0] = LCP[0];
  cilk_for(intT i=1;i<n;i++){
    leafSuperNodeID[i] = (LCP[i] > LCP[i-1]) ? 
      superNodeID[i] : superNodeID[i-1];
    leafDepth[i] = (LCP[i] > LCP[i-1]) ? LCP[i] : LCP[i-1];
  }

  delete [] superNodeID;

  nextTime("representatives");

  node* nodes = newA(node,n+nm+1);
  //dummy
  nodes[n+nm].parent = n+nm;
  nodes[n+nm].value = 0;
  cilk_for(intT i=0;i<nm;i++){
    intT index = parentPlusDirectionPairs[oout[i]].second;
    intT a = parents[index];
    if(a >= 0) nodes[i].parent = a;
    else nodes[i].parent = n+nm;
    nodes[i].value = LCP[index];
  }

  cilk_for(intT i=0;i<n;i++){
    nodes[i+nm].parent = leafSuperNodeID[i];
    nodes[i+nm].value = n-SA[i]+1;
  }
  
  // stNode<int>* nodes = new stNode<int>[n+nm];
  // //internal nodes
  // cilk_for(int i=0;i<nm;i++){
  //   int index = parentPlusDirectionPairs[oout[i]].second;
  //   int a = parents[index];
  //   if(a >= 0){ //pointing to non-root
  //     int depth = LCP[a];
  //     nodes[i].setValues(superNodeID[a],s[SA[index]+depth],
  // 			 superNodeID[index],SA[index]+depth,
  // 			 LCP[index]-LCP[a]);
  //   }
  // }
  // //leaves
  // cilk_for(int i=0;i<n;i++){
  //   nodes[i+nm].setValues(leafSuperNodeID[i],s[SA[i]+leafDepth[i]],-1,
  // 			  SA[i]+leafDepth[i],n-SA[i]-leafDepth[i]+1);
  // }
  //delete all information here
  delete [] leafSuperNodeID;
  delete [] leafDepth;
  delete [] parents;
  delete [] parentPlusDirectionPairs;
  delete [] oout;  

  delete [] SA;
  delete [] LCP;

  nextTime("time for computing node information");
  return suffixTree(0,0,NULL,NULL);
  suffixTree ST = suffixTree(n,n+nm+1,nodes,s);

  // //hash table
  // stNodeTable outgoingEdges = makeStNodeTable(5*(n+nm)/8);

  // cilk_for(int i=0;i<nm+n;i++){
  //   outgoingEdges.insert(&nodes[i]);
  // }
  
  nextTime("time for inserting into hash table (different from merge version)");
  return ST;
  // return suffixTree(n,n+nm,outgoingEdges,s,nodes);
}
