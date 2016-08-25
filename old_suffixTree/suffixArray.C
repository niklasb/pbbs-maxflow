#include <iostream>
#include "gettime.h"
#include "sequence.h"
#include "intSort.h"
#include "cilk.h"
#include "merge.h"
#include "rangeMin.h"
#include "stackSpace.h"
using namespace std;

bool isSorted(intT *SA, intT *s, intT n);
typedef pair<intT,intT> ipair;

// Radix sort a pair of integers based on first element
void radixSortPair(ipair *A, intT n, long m, stackSpace* stack) {
  char* s = stack->push(intSort::iSortSpace<ipair>(n));
  intSort::iSort(A,NULL,n,m,false,s,utils::firstF<intT,intT>());
  stack->pop(1);
}

inline bool leq(intT a1, intT a2,   intT b1, intT b2) {
  return(a1 < b1 || a1 == b1 && a2 <= b2); 
}                                                  

inline bool leq(intT a1, intT a2, intT a3, intT b1, intT b2, intT b3) {
  return(a1 < b1 || a1 == b1 && leq(a2,a3, b2,b3)); 
}

struct compS {
  intT* _s;
  intT* _s12;
  compS(intT* s, intT* s12) : _s(s), _s12(s12) {}
  intT operator () (intT i, intT j) {
    if (i%3 == 1 || j%3 == 1) 
      return leq(_s[i],_s12[i+1], _s[j],_s12[j+1]);
    else
      return leq(_s[i],_s[i+1],_s12[i+2], _s[j],_s[j+1],_s12[j+2]);
  }
};

struct mod3is1 { bool operator() (intT i) {return i%3 == 1;}};

inline intT computeLCP(intT* LCP12, intT* rank, myRMQ & RMQ, 
		      intT j, intT k, intT* s, intT n){
 
  intT rank_j=rank[j]-2;
  intT rank_k=rank[k]-2;
  if(rank_j > rank_k) {swap(rank_j,rank_k);} //swap for RMQ query

  intT l = ((rank_j == rank_k-1) ? LCP12[rank_j] 
	   : LCP12[RMQ.query(rank_j,rank_k-1)]);

  intT lll = 3*l;
  if (s[j+lll] == s[k+lll]) {
    if (s[j+lll+1] == s[k+lll+1]) return lll + 2;
    else return lll + 1;
  } 
  return lll;
}

timer radixTime;
timer mergeTime;
timer LCPtime;

// This recursive version requires s[n]=s[n+1]=s[n+2] = 0
// K is the maximum value of any element in s
pair<intT*,intT*> suffixArrayRec(intT* s, intT n, intT K, bool findLCPs,
			       stackSpace* stack) {
  //cout << "n = " << n << endl;
  n = n+1;
  intT n0=(n+2)/3, n1=(n+1)/3, n12=n-n0;
  ipair *C = (ipair *) stack->push(n12*sizeof(ipair));

  intT bits = utils::logUp(K);
  // if 3 chars fit into an int then just do one radix sort
  if (bits < 11) {
    cilk_for (intT i=0; i < n12; i++) {
      intT j = 1+(i+i+i)/2;
      C[i].first = (s[j] << 2*bits) + (s[j+1] << bits) + s[j+2];
      C[i].second = j;}
    radixTime.start();
    radixSortPair(C, n12, 1 << 3*bits, stack); 
    radixTime.stop();

  // otherwise do 3 radix sorts, one per char
  } else {
    cilk_for (intT i=0; i < n12; i++) {
      intT j = 1+(i+i+i)/2;
      C[i].first = s[j+2]; 
      C[i].second = j;}
    radixTime.start(); 
    // radix sort based on 3 chars
    radixSortPair(C, n12, K, stack); 
    cilk_for (intT i=0; i < n12; i++) C[i].first = s[C[i].second+1];
    radixSortPair(C, n12, K, stack); 
    cilk_for (intT i=0; i < n12; i++) C[i].first = s[C[i].second];
    radixSortPair(C, n12, K, stack);
    radixTime.stop();

  }

  // copy sorted results into sorted12
  intT* name12 = (intT*) stack->push(sizeof(intT)*n12); 
  intT* sorted12 = (intT*) stack->push(sizeof(intT)*n12); 
  cilk_for (intT i=0; i < n12; i++) sorted12[i] = C[i].second;

  // generate names based on 3 chars
  cilk_for (intT i = 1;  i < n12;  i++) {
    name12[i] = (s[sorted12[i]]   != s[sorted12[i-1]] ||
		 s[sorted12[i]+1] != s[sorted12[i-1]+1] ||
		 s[sorted12[i]+2] != s[sorted12[i-1]+2]);
  }
  name12[0] = 1;
  sequence::scanI(name12,name12,n12,utils::addF<intT>(),(intT)0);
  intT names = name12[n12-1];
  
  pair<intT*,intT*> SA12_LCP;
  intT* SA12;
  intT* LCP12 = NULL;
  // recurse if names are not yet unique
  if (names < n12) {
    intT* s12  = (intT*) stack->push(sizeof(intT)*(n12+3));
    s12[n12] = s12[n12+1] = s12[n12+2] = 0;

    // move mod 1 suffixes to bottom half and and mod 2 suffixes to top
    cilk_for (intT i= 0; i < n12; i++)
      if (sorted12[i]%3 == 1) s12[sorted12[i]/3] = name12[i];
      else s12[sorted12[i]/3+n1] = name12[i];
    s12 = (intT*) stack->popKeepTop(3); // delete C, name12, sorted12; keep s12

    SA12_LCP = suffixArrayRec(s12, n12, names+1, findLCPs, stack); 
    SA12 = SA12_LCP.first;
    LCP12 = SA12_LCP.second;

    // restore proper indices into original array
    cilk_for (intT i = 0;  i < n12;  i++) {
      intT l = SA12[i]; 
      SA12[i] = (l<n1) ? 3*l+1 : 3*(l-n1)+2;
    }
  } else {
    sorted12 = (intT*) stack->popKeepTop(2); // delete C, name12; keep sorted12
    SA12 = sorted12; // suffix array is sorted array
    if (findLCPs) {
      LCP12 = (intT*) stack->push(sizeof(intT)*(n12+3));
      cilk_for(intT i=0;i<n12+3;i++) 
	LCP12[i]=0; //LCP's are all 0 if not recursing
    }
  }
  // at this point s and SA12 (and LCP if findLCP) should be on the stack

  // place ranks for the mod12 elements in full length array
  // mod0 locations of rank will contain garbage
  intT* rank  = (intT *) stack->push(sizeof(intT) * (n+2));
  rank[n]=1; rank[n+1] = 0;
  cilk_for (intT i = 0;  i < n12;  i++) {rank[SA12[i]] = i+2;}
  
  // stably sort the mod 0 suffixes 
  // uses the fact that we already have the tails sorted in SA12
  intT* s0  = (intT*) stack->push(sizeof(intT) * n0);
  intT x = sequence::filter(SA12,s0,n12,mod3is1());
  ipair *D = (ipair *) stack->push(n0*sizeof(ipair));
  D[0].first = s[n-1]; D[0].second = n-1;
  cilk_for (intT i=0; i < x; i++) {
    D[i+n0-x].first = s[s0[i]-1]; 
    D[i+n0-x].second = s0[i]-1;}
  radixTime.start();
  radixSortPair(D,n0, K, stack);
  radixTime.stop();
  intT* SA0  = s0; // reuse memory since not overlapping
  cilk_for (intT i=0; i < n0; i++) SA0[i] = D[i].second;
  stack->pop(1);  // delete D

  compS comp(s,rank);
  intT o = (n%3 == 1) ? 1 : 0;
  intT *SA = (intT*) stack->push(n*sizeof(intT));
  mergeTime.start();
  merge(SA0+o,n0-o,SA12+1-o,n12+o-1,SA,comp);
  mergeTime.stop();

  if(!findLCPs){
    SA = (intT*) stack->popKeepTop(4); // delete s, SA12, rank, SA0; keep SA
    return make_pair(SA,(intT*) NULL);
  } else {
    //get LCP from LCP12
    intT* LCP = (intT*) stack->push(sizeof(intT)*n);
    LCP[n-1] = LCP[n-2] = 0; 
    LCPtime.start();
    myRMQ RMQ(LCP12,n12+3); //simple rmq
    cilk_for(intT i=0;i<n-2;i++){ 
      intT j=SA[i];
      intT k=SA[i+1];
      intT CLEN = 16;
      intT ii;
      for (ii=0; ii < CLEN; ii++) 
	if (s[j+ii] != s[k+ii]) break;
      if (ii != CLEN) LCP[i] = ii;
      else {
      	if (j%3 != 0 && k%3 != 0)  
	  LCP[i] = computeLCP(LCP12, rank, RMQ, j, k, s, n); 
	else if (j%3 != 2 && k%3 != 2)
	  LCP[i] = 1 + computeLCP(LCP12, rank, RMQ, j+1, k+1, s, n);
	else 
	  LCP[i] = 2 + computeLCP(LCP12, rank, RMQ, j+2, k+2, s, n);
	  }
    }
    LCPtime.stop();
    // delete s, LCP12, SA12, rank, SA0; keep SA and LCP
    stack->popKeepTop(5,2); 
    SA = (intT*) stack->ith(1);
    LCP = (intT*) stack->ith(0);
    
    return make_pair(SA,LCP);
  }
}

pair<intT*,intT*> suffixArray(intT* s, intT n, bool findLCPs, stackSpace* stack) {
  startTime();
  intT *ss = (intT*) stack->push(sizeof(intT)*(n+3));
  ss[n] = ss[n+1] = ss[n+2] = 0;
  cilk_for (intT i=0; i < n; i++) ss[i] = s[i]+1;
  intT k = 1 + sequence::reduce(ss,n,utils::maxF<intT>());

  radixTime.clear();
  mergeTime.clear();
  LCPtime.clear();

  pair<intT*,intT*> SA_LCP = suffixArrayRec(ss,n,k, findLCPs, stack);

  cout << "Radix sort time: " << radixTime.total() << endl;
  cout << "Merge time: " << mergeTime.total() << endl;
  cout << "LCP time: " << LCPtime.total() << endl;

  return SA_LCP;
}

intT* suffixArrayNoLCP(intT* s, intT n, stackSpace* stack) { 
  return suffixArray(s,n,false,stack).first;}
