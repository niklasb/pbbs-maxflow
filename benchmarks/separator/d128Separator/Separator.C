// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
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

#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
#include "maxMatching.h"
#include "separatorHash.h"
#include "weightedArc.h"
using namespace std;
#define DETAILED_TIME 1

timer matchingTime;
timer insertionTime;
timer RCTime;
timer clearTime;
timer initTime;

void reportDetailedTime() {
  initTime.reportTotal("  init time");
  matchingTime.reportTotal("  matching time");
  insertionTime.reportTotal("  hash table insertion time");
  clearTime.reportTotal("  hash table clearing time");
  RCTime.reportTotal("  reservations & commit time");
}


struct arcF {
  ArcTable T;
  uintT* Parent;
  arcF(ArcTable & _T, uintT* _Parent) : T(_T), Parent(_Parent) {}
  void operator () (weightedArc a) { 
    weightedArc b(Parent[a.u()],Parent[a.v()],a.weight); 
    if(b.u() != b.v()) T.insert(b);
  }
};

struct contractResult {
  uintT n;
  uintT m;
  uintT* vWeights;
  contractResult(uintT _n, uintT _m, uintT* _vWeights) : 
    n(_n), m(_m), vWeights(_vWeights) {}
};

// **************************************************************
//    SEPARATOR
// **************************************************************

// Contracts a weighted graph G along a set of edges given by a matching
// We refer to the new contracted node as the parent and the two old
//   nodes as the left and right children.
// M is the matching by which it contracts
//    each localtion in M (1 per vertex) points to the other end 
//    of the matching, or is negative if unmatched
// P is an ouput.   It is an array on the vertices pointing to their
//   parent in the new resulting graph.  It is used by uncontractGraph.

contractResult contractGraph(uintT n, uintT m, uintT* M, uintT* Parent,ArcTable & T0, ArcTable & T1, uintT* vertexWeights) {
  parallel_for(uintT i=0;i<n;i++) {
    if(M[i] == UINT_T_MAX || M[i] < i) { Parent[i] = 1; }
    else { Parent[i] = 0; }
  }

  uintT count = sequence::scan(Parent,Parent,n,utils::addF<uintT>(),(uintT)0);
  uintT* vWeights = newA(uintT,count);
  parallel_for(uintT i=0;i<n;i++) {
    if(M[i] == UINT_T_MAX || M[i] < i) vWeights[Parent[i]] = vertexWeights[i];
  }
  parallel_for (uintT i=0; i < n; i++) 
    if (M[i] != UINT_T_MAX && M[i] > i) { Parent[i] = Parent[M[i]]; vWeights[Parent[i]]+=vertexWeights[i];}
  //utils::writeAdd(&vWeights[Parent[i]], vertexWeights[i]);}

  //VERIFY THAT WE DON'T NEED WRITEADD ABOVE
  
  clearTime.start();
  T1.makeSmaller(m);
  clearTime.stop();

  insertionTime.start();
  T0.map(arcF(T1,Parent));
  insertionTime.stop();
  //insertionTime.reportTotal("  hash table insertion time");
  //exit(0);
  return contractResult(count,T1.count(),vWeights);
}


// Undoes the contraction done by contract Graph.
// In particular given the array of integers Offset for the contracted graph
//  it gives the left child of location i, Offset[i]
//  and the right child, Offset[i]+ W[LeftChild[i]]
// Basically a parallel prefix operation over the contraction tree
uintT* uncontractGraph(uintT n, uintT* vertexWeights, uintT* Offset, uintT* M, uintT* Parent) {
  uintT* R = newA(uintT,n);
  parallel_for(uintT i = 0; i < n; i++) {
    if (M[i] == UINT_T_MAX|| M[i] < i) R[i] = Offset[Parent[i]];
    else R[i] = Offset[Parent[i]] + vertexWeights[M[i]];
  }
  return R;
}

struct reservationElt {
  uint priority;
  float ratio;
  reservationElt(uint _i, float _f) : priority(_i), ratio(_f) {}
};

inline bool writeMax(reservationElt *a, reservationElt b) {
  reservationElt c = *a; bool r=0;
  do c = *a;
  while ((c.ratio < b.ratio || ((c.ratio == b.ratio) && 
				(c.priority < b.priority))) 
  	 && !(r=utils::CAS(a,c,b)));
  return r;
}

struct reserve {
  uintT* vertexWeights;
  reservationElt* reservations;
  reserve(reservationElt* _reservations, uintT* _vertexWeights) : 
    reservations(_reservations), vertexWeights(_vertexWeights) {}
  void operator () (weightedArc a, uintT i) {
    uint u = a.u(); uint v = a.v();
    float ratio = 
      float(a.weight)/(float(vertexWeights[u])*float(vertexWeights[v]));
    reservationElt re(i,ratio);
    writeMax(&reservations[u],re);
    writeMax(&reservations[v],re);
  }
};

struct commit {
  uintT* T;
  reservationElt* reservations;  
  uintT* vertexWeights;
  commit(reservationElt* _reservations, uintT* _T) : 
    reservations(_reservations), T(_T) {}
  void operator () (weightedArc a, uintT i) { 
    uint u = a.u(); uint v = a.v();
    if(reservations[u].priority == i) T[u] = v;
    if(reservations[v].priority == i) T[v] = u;
  }
};

uintT* Matching(ArcTable & T0, uintT* vertexWeights, uintT n) {
  uintT* T = newA(uintT,n);
  reservationElt* reservations = newA(reservationElt,n);
  parallel_for(uintT i=0;i<n;i++) { 
    reservations[i] = reservationElt(UINT_MAX,0.0);
    T[i] = UINT_T_MAX; }
  RCTime.start();
  T0.mapIndex(reserve(reservations,vertexWeights));
  T0.mapIndex(commit(reservations,T));
  RCTime.stop();
  free(reservations); 

  // Generate a maximal matching on the links in T
  matchingTime.start();
  uintT* M = maximalMatchingT(T,n);
  matchingTime.stop();
  free(T);

  return M;
}  

#define MAX_ROUNDS 200

// The main separator loop
// It switches back and forth between two arrays for holding the edges.
uintT* separatorR(uintT* vertexWeights, uintT n, uintT m, ArcTable T0, ArcTable T1, uintT round) {
  cout << "Separator: size = " << n << " m = "<< m<<" round = " << round << endl;
  if (n == 1 || m == 0 || round > MAX_ROUNDS) {
    uintT* R = newA(uintT,n); 
    uintT count=0;
    for (uintT i=0; i < n; i++) {R[i] = count; count+=vertexWeights[i];}
    cout << "Separator rounds = " << round << " n="<<n<<" m="<<m<<endl;
    return R;
  }
  else {
    double t1, t2;
    
    // find a matching based on edge and vertex weights
    uintT* M = Matching(T0,vertexWeights,n); 

    // array of parent pointers for contractGraph
    uintT* Parent = newA(uintT,n);
    contractResult CR = contractGraph(n,m,M, Parent, T0, T1, vertexWeights);
    
    // recurse on contracted graph
    uintT* R = separatorR(CR.vWeights, CR.n,CR.m, T1, T0, round+1);
    
    // uncontract and generate ordering in RR
    uintT* RR = uncontractGraph(n, vertexWeights, R, M, Parent);
    free(R);
    free(Parent); free(M); free(CR.vWeights);
    return RR;
  }
}


uintT* separator(edgeArray<uintT> G) {
  uintT n = max<uintT>(G.numCols,G.numRows);
  uintT m = G.nonZeros;
  cout<<"n="<<n<<" m="<<m<<endl;

  initTime.start();

  uintT* vertexWeights = newA(uintT,n);
  parallel_for(uintT i=0;i<n;i++) vertexWeights[i] = 1;

  ArcTable T0(m,1.33);
  ArcTable T1(m,1.33);
  
  //insert initial edges into T0
  parallel_for(uintT i=0;i<m;i++) { 
    if(G.E[i].u < G.E[i].v) { 
      weightedArc a(G.E[i].u,G.E[i].v,1);
      T0.insert(a);
    } else if(G.E[i].u > G.E[i].v) {
      weightedArc a(G.E[i].v,G.E[i].u,1);
      T0.insert(a);
    }
  }
 
  initTime.stop();

  uintT *R = separatorR(vertexWeights, n,m,T0,T1, (uintT)0);
  T0.del(); T1.del();  free(vertexWeights);
  if (DETAILED_TIME) reportDetailedTime();

  return R;
}

// typedef pair<double,intT> diPair;

// struct edgeLessPair{
//   bool operator() (diPair a, diPair b){
//     return (a.second < b.second); }
// };


// struct addDiPair { diPair operator() (diPair a, diPair b){
//   b.first=a.first+b.first;return b;
//   }
// };

// struct minDiPair { diPair operator() (diPair a, diPair b){
//   return (a.first < b.first) ? a : b;
// }
// };

// intT bestCut(graph<intT> G){
//   diPair* edges = newA(diPair,2*G.m);
//   intT* offsets = newA(intT,G.n);
  
//   parallel_for(intT i=0;i<G.n;i++){
//     offsets[i] = G.V[i].degree<<1;
//   }

//   sequence::scan(offsets,offsets,G.n,utils::addF<intT>(),(intT)0);

//   parallel_for(intT i=0;i<G.n;i++){
//     intT o = offsets[i];
//     for(intT j=0;j<G.V[i].degree;j++){
//       intT jj = j<<1;
//       if(G.V[i].Neighbors[j] > i){
// 	edges[o+jj].first = 1;
// 	edges[o+jj+1].first = -1;
//       } else {
// 	edges[o+jj].first = 0;
// 	edges[o+jj+1].first = 0;
//       }
//       edges[o+jj].second = i;
//       edges[o+jj+1].second = G.V[i].Neighbors[j];
      
//     }
//   }
 
//   intSort::iSort(edges,2*G.m,G.n,utils::secondF<double,intT>());
  
//   sequence::scan(edges,edges,2*G.m,addDiPair(),make_pair(0.0,(intT)-1));

//   //get max at offset points
//   diPair* cuts = newA(diPair,G.n-1);
//   parallel_for(intT i=1;i<G.n;i++){
//     intT ii=i-1;
//     cuts[ii] = edges[offsets[i]];
//     cuts[ii].first /= i;
//     cuts[ii].first /= G.n-i;
//   }
//   free(offsets);
//   diPair best = sequence::reduce(cuts,G.n-1,minDiPair());

//   cout<<"Sparsest cut position: cut after position " << best.second
//       << " Cost: " << best.first << endl;

//   free(cuts); free(edges);
//   return best.second;
// }

// // number of off diagonals for diagonal blocks of size blockSize
// double sepSize(graph<intT> G, intT blockSize) {
//   double k = 0;
//   for (intT i = 0; i < G.n; i++)
//     for (intT j = 0; j < G.V[i].degree; j++) 
//       if ((i / blockSize) != (G.V[i].Neighbors[j] / blockSize)) k+= 1;
//   return k;
// }

// double logCost(graph<intT> G) {
//   double logs = 0.0;
//   for (intT i = 0; i < G.n; i++) { 
//     for (intT j = 0; j < G.V[i].degree; j++) {
//       double logcost = log(((double) abs(i - G.V[i].Neighbors[j])) + 1);
//       logs += logcost;
//     }
//   }
//   return logs/(G.m*log(2.0));
// }

void test(edgeArray<uintT> G) {
  // double s8 = sepSize(G, 1<<8);
  // double s16 = sepSize(G, 1<<16);
  // double lc = logCost(G); 
  //graphCheckConsistency(G);
  //randomly reorder
  //G = graphReorder(G,(intT*)NULL);
  // for(int i=0;i<G.n;i++){
  //   cout<<i<<": ";
  //   for(int j=0;j<G.V[i].degree;j++) cout<<G.V[i].Neighbors[j]<<" ";
  //   cout<<endl;
  // }

  startTime();
  uintT* I = separator(G);
  //stopTime(weight,str);
  nextTime("separator");

  // Check correctness
  //checkOrder(I, G.numRows);

  // Check block separator sizes with reordering
  //graph<intT> GG = graphReorder(G,I);
  //graphCheckConsistency(GG);


  // if (0) {
  //   cout << setprecision(3) << "  Off Diagonal (256) = " 
  // 	 << (100.*s8)/GG.m << "% before, "
  // 	 << (100.*sepSize(GG, 1<<8))/GG.m << "% after" << endl;
  //   cout << "  Off Diagonal (64K blocks) = " 
  // 	 << (100.*s16)/GG.m << "% before, "
  // 	 << (100.*sepSize(GG, 1<<16))/GG.m << "% after" << endl;
  // }

  // //cout << "Cut location: " << cut << " Cut cost: " << cutCost(GG,cut) << endl;

  // cout << "  Quality: Log Cost = " 
  //      << lc << " before, " 
  //      << logCost(GG) << " after" << endl;

  // startTime();
  // bestCut(GG);
  // nextTime("Sparsest cut computation time: ");

  free(I);
  //GG.del();
}



