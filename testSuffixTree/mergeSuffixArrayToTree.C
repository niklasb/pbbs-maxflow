#include <iostream>
#include "gettime.h"
#include "blockRadixSort.h"
#include "sequence.h"
#include "utils.h"
#include "suffixTree.h"
#include "cartesianTree.h"


using namespace std;
#define CHECK 0

//void cartesianTreeStack(node_c* Nodes, intT n);

inline ulong getRoot(node* nodes, ulong i) {
  ulong root = nodes[i].parent;
  while (root != 0 && nodes[nodes[root].parent].value == nodes[root].value)
    root = nodes[root].parent;
  return root;
}

// // NOT CURRENTLY USED (NOR CHECKED FOR CORRECTNESS)
// uintT filterClusterRoots(node* nodes, node* out, uintT m) {
//   uintT* nums = new uintT[m];
//   nums[0] = 1;
//   cilk_for(uintT i = 1; i < m; i++){
//     uintT p = nodes[i].parent;
//     nums[i] = (nodes[i].value > nodes[p].value);
//     nodes[i].parent = getRoot(nodes, i);
//   }  
//   uintT l = sequence::scan(nums, nums, m, utils::addF<int>(), (uintT)0);

//   out = new node[l];
//   // should be cilk_for, but broken
//   for(uintT i = 0; i < m; i++){
//     uintT e = (i == m-1) ? l : nums[i+1];
//     if (nums[i] < e) {
//       out[nums[i]].value = nodes[i].value;
//       out[nums[i]].parent = nums[nodes[i].parent];
//     }
//   }
//   free(nums);
//   return l;
// }

void computeStatistics(node* nodes, long n, char* statsFile) {
  //compute statistics
  //startTime();
  //intT* stats = memory;

  uintT* stats = newA(uintT,n);
  cilk_for(long i=0;i<n;i++){
    long j = 2*i+1;
    stats[i] = nodes[nodes[j].parent].value;  
  }

  long maxV = 1+sequence::reduce(stats,n,utils::maxF<uintT>());

  intSort::iSort(stats,n,maxV,utils::identityF<uintT>());

  long maxlength = stats[n-1]+1;

  //intT* offsets = memory+n;
  uintT* offsets = newA(uintT,maxlength+1);
  offsets[maxlength] = n;
  cilk_for(long i=0;i<maxlength;i++) offsets[i] = UINT_T_MAX;
  offsets[stats[0]] = 0;

  cilk_for(long i=1;i<n;i++) {
    if(stats[i] != stats[i-1]) offsets[stats[i]] = i;
  }

  free(stats);
 
  sequence::scanBack(offsets, offsets, maxlength+1, 
		     utils::minF<uintT>(), (uintT)UINT_T_MAX);
 
  //intT* buckets = memory+2*n;
  uintT* buckets = newA(uintT,maxlength);

  cilk_for(long i=1;i<maxlength;i++) buckets[i] = offsets[i] - offsets[i-1];

  free(offsets);   free(buckets);
  //nextTime("Computing statistics (subtract off)");
  // stringstream toWrite;
  // for(intT i=1;i<maxlength;i++) toWrite<<i<<" "<<buckets[i]<<endl;

  // ofstream oFile(statsFile);
  // oFile<<toWrite.str();
  // oFile.close();

}

typedef pair<uintT,uintT> ipair;

struct maxPairF { ipair operator() (const ipair& a, const ipair& b) const {return (a.first>b.first) ? a : b;}};

struct toInt { uintT operator() (bool a) {return (uintT) a;}};

//#define INTS_PER_LINE 1

void fillIn(node* nodes, ipair* Sizes, long n){
  cilk_for(long i=0;i<n;i++) { Sizes[i].first = 0; Sizes[i].second = i; }
  cilk_for(long i=0;i<n;i++) {
    uintT v = 1;
    long j = 2*i + 1;
    long parent = nodes[j].parent/2;

    while(parent != 0 && !utils::CAS(&Sizes[parent].first,(uintT) 0, v)){
      Sizes[parent].first += v;
      v = Sizes[parent].first;
      parent = nodes[2*parent].parent/2;
    }
  }
  bool* Fl = newA(bool,n);
  Fl[0] = 0;
  cilk_for(long i=1;i<n;i++) {
    long id = Sizes[i].second;
    long parent = nodes[id].parent;
    Fl[i] = nodes[parent].value < nodes[id].value ? 1 : 0;
  }


  long m = sequence::pack(Sizes, Sizes, Fl, n);
  free(Fl);

}

void computeSubtreeSize(node* nodes, long n, char* statsFile) {
  pair<uintT,uintT> *Sizes = (pair<uintT,uintT> *) malloc(n*sizeof(pair<uintT,uintT>));

  //Sizes[0] not used, value always 0
  fillIn(nodes,Sizes,n);
    //cout<<"m="<<m<<endl;
  // long maxV = 1+sequence::reduce(Sizes,n,maxPairF()).first;
  // intSort::iSort(Sizes,NULL,n,maxV,false,utils::firstF<intT,intT>());

  //intSort::iSort(Sizes,n,maxV,utils::identityF<intT>());
  //free(Sizes);

  //for(int i=n-1;i>=n-100;i--)cout<<"("<<Sizes[i].first<<","<<Sizes[i].second<<") ";
  //cout<<endl;
  free(Sizes); 

}


suffixTree suffixArrayToTree (uintT* SA, uintT* LCP, long n, uintT* s){
  startTime();
  // if (0) {
  //   node_c* test = new node_c[2*n];
  //   cilk_for(intT i=1; i<n; i++){ //internal
  //     test[2*i].value = LCP[i-1];
  //     test[2*i+1].value = n-SA[i]+1;
  //     test[2*i].rightChild = test[2*i+1].rightChild = 
  // 	test[2*i].parent = test[2*i+1].parent = -1;
  //   }
  //   test[0].value = 0;
  //   test[1].value = n-SA[0]+1;
  //   test[0].parent = test[1].parent = -1;
  
  //   nextTime("Time to initialize nodes for sequential CT (subtract off)");
  //   cartesianTreeStack(test+1,2*n-1);
  //   nextTime("Time for stack-based sequential CT construction (subtract off)");
  // }

  startTime();
  long m = 2*n;

  //initialize nodes
  node* nodes = newA(node,m);
  //node* nodes = (node*) stack->push(sizeof(node)*m);
  cilk_for(long i=1; i<n; i++){ 
    nodes[2*i].value = LCP[i-1];
    nodes[2*i+1].value = n-SA[i]+1; // length of string including 1 past end
    nodes[2*i].parent = 0;
    nodes[2*i+1].parent = 0;
  }
  nodes[0].value = 0;
  nodes[1].value = n-SA[0]+1;
  nodes[0].parent = nodes[1].parent = 0;
 
  free(LCP); free(SA);
  //nextTime("Time to initialize nodes");
  
  cartesianTree(nodes, 1, m-1);
  //for(int i=0;i<m;i++)cout<<"("<<nodes[i].value<<","<<nodes[i].parent<<") ";cout<<endl;
  //nextTime("Time for building CT in parallel");

  if(1){
    //computeSubtreeSize(nodes,n,(char*)"a.tmp");
    //nextTime("Time for computing subtree size and depth (subtract off)");
  }

  // shortcut to roots of each cluster
  cilk_for(long i = 1; i < m; i++) 
    nodes[i].parent = getRoot(nodes, i);
  //nextTime("Time for shortcuts");
  //for(int i=0;i<m;i++)cout<<"("<<nodes[i].value<<","<<nodes[i].parent<<") ";cout<<endl;

  //intT* memory = (intT*)stack->push(3*n*sizeof(intT));
  if(1) {
    //computeStatistics(nodes,n,(char*)"a.tmp");
    //nextTime("Time for computing statistics (subtract off)");
  }
  //stack->pop(1);
  // insert into hash table
  
  suffixTree ST = suffixTree(n, m, nodes, s);
  //nextTime("Time for inserting into hash table");

  return ST;
}
