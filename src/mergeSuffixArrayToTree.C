#include <iostream>
#include "gettime.h"
#include "intSort.h"
#include "sequence.h"
#include "utils.h"
#include "hash.h"
#include "suffixTree.h"
#include "cartesianTree.h"

using namespace std;
#define CHECK 0

inline int getRoot(node* nodes, int i) {
  int root = nodes[i].parent;
  while (root != 0 && nodes[nodes[root].parent].value == nodes[root].value)
    root = nodes[root].parent;
  return root;
}

inline void setNode(node *nodes, stNode<int>* stnodes, int *s,
		    int i, int j, int position) {
  int parent = nodes[j].parent;
  int parentDepth = nodes[parent].value;
  int offset = position + parentDepth;
  int length = nodes[j].value - parentDepth;
  stnodes[i].setValues(parent,s[offset],j,offset,length);
}

suffixTree suffixArrayToTree (int* SA, int* LCP, int n, int* s){
  startTime();

  //initialize nodes
  node* nodes = new node[2*n];
  cilk_for(int i=1; i<n; i++){ //internal
    nodes[2*i].value = LCP[i-1];
    nodes[2*i+1].value = n-SA[i]+1;
    nodes[2*i].parent = nodes[2*i+1].parent = 0;
  }
  nodes[0].value = 0;
  nodes[1].value = n-SA[0]+1;
  nodes[0].parent = nodes[1].parent = 0;
  delete [] SA;  delete [] LCP;
  nextTime("Time to initialize nodes");

  cartesianTree(nodes,1,2*n-1);
  nextTime("Time for building tree");

  // Filter out non-root internal nodes
  int* flags = new int[n];
  flags[0] = -1;
  cilk_for(int i=1; i<n;i++){
    int j = 2*i;
    int p = nodes[j].parent;
    if (nodes[j].value > nodes[p].value) flags[i] = j;
    else flags[i] = -1;
  }  
  int* oout = new int[n];
  int nm = sequence::filter(flags,oout,n,notNeg());
  delete flags;
  nextTime("Time for filter");
  //cout << "nm=" << nm << endl;

  // shortcut to roots of each cluster
  cilk_for(int i=1;i<2*n;i++)
    nodes[i].parent = getRoot(nodes, i);
  nextTime("Time for shortcuts");

  // copy leaves to hash structure
  stNode<int>* stnodes = new stNode<int>[n+nm];
  cilk_for(int i=0;i<n;i++){
    int j = 2*i+1;
    setNode(nodes,stnodes,s,i,j,(n-nodes[j].value+1));
  }

  // copy internal nodes to hash structure
  cilk_for(int i=0;i<nm;i++){
    int j = oout[i];
    setNode(nodes,stnodes,s,n+i,j,(n-nodes[j-1].value+1));
  }
  delete oout;  delete nodes;
  nextTime("Time to copy data to table entries");

  // insert into hash table
  stNodeTable table = makeStNodeTable(3*(n+nm)/4);
  stNodeTable* ST = &table;
  cilk_for(int i=0; i<n+nm; i++) {
    if (CHECK) utils::myAssert(ST->find(stnodes+i)==NULL,
			       "SuffixArrayToTree: repeated entry");
    ST->insert(stnodes+i);
  }
  nextTime("Time for inserting into hash table");

  return suffixTree(n,n+nm,table,s,stnodes);
}
