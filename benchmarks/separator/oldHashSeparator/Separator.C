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
#include "blockRadixSort.h"
#include "graphUtils.h"
#include "deterministicHash.h"
#include "separatorHash.h"
#include "weightedArc.h"
using namespace std;

#define DETAILED_TIME 1

timer matchingTime;
timer insertionTime;
timer weightTime;
timer reservationTime;
timer packTime;
timer commitTime;
timer initTime;
timer reassignTime;

void reportDetailedTime() {
  initTime.reportTotal("  init time");
  matchingTime.reportTotal("  matching time");
  weightTime.reportTotal("  weight time");
  reassignTime.reportTotal("  reassign time");
  insertionTime.reportTotal("  hash table insertion time");
  packTime.reportTotal("  hash table packing time");
  reservationTime.reportTotal("  reservations time");
  commitTime.reportTotal("  commit time");
}

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

intT* contractGraph(intT* M, intT* Parent, weightedArcArray &EE, weightedArcArray &EEO, intT* vertexWeights) {
  weightedArc* E = EE.E;
  weightedArc* EO = EEO.E;
  intT n = EE.n;
  intT m = EE.m;
  parallel_for(intT i=0;i<n;i++) {
    if(M[i] == INT_T_MAX || M[i] < i) { Parent[i] = 1; }
    else { Parent[i] = 0; }
  }

  intT count = sequence::scan(Parent,Parent,n,utils::addF<intT>(),(intT)0);
  intT* vWeights = newA(intT,count);
  parallel_for(intT i=0;i<n;i++) {
    if(M[i] == INT_T_MAX || M[i] < i) vWeights[Parent[i]] = vertexWeights[i];
  }
  parallel_for (intT i=0; i < n; i++) 
    if (M[i] != INT_T_MAX && M[i] > i) { Parent[i] = Parent[M[i]]; utils::writeAdd(&vWeights[Parent[i]], vertexWeights[i]);}

  reassignTime.start();
  parallel_for(intT i=0;i<m;i++)
    E[i] = weightedArc(Parent[E[i].u()],Parent[E[i].v()],E[i].weight);
  reassignTime.stop();

  insertionTime.start();
  ArcTable AT(m);
  weightedArc** Eptr = newA(weightedArc*,m);
  parallel_for(intT i=0;i<m;i++) 
    Eptr[i] = &E[i];

  parallel_for(intT i=0;i<m;i++) 
    if(E[i].u() != E[i].v()) AT.insert(Eptr[i]);
  insertionTime.stop();

  //AT.print();
  packTime.start();
  _seq<weightedArc*> Eseq = AT.entries();

  intT mm = Eseq.n;
  //cout<<"mm = "<<mm<<endl;
  parallel_for(intT i=0;i<mm;i++) EEO.E[i] = *Eseq.A[i];

  EEO.m = mm;
  EEO.n = count;
  packTime.stop();

  AT.del(); free(Eptr);
  return vWeights;
}


// Undoes the contraction done by contract Graph.
// In particular given the array of integers Offset for the contracted graph
//  it gives the left child of location i, Offset[i]
//  and the right child, Offset[i]+ W[LeftChild[i]]
// Basically a parallel prefix operation over the contraction tree
intT* uncontractGraph(intT n, intT* vertexWeights, intT* Offset, intT* M, intT* Parent) {
  intT* R = newA(intT,n);
  parallel_for(intT i = 0; i < n; i++) {
    if (M[i] == INT_T_MAX|| M[i] < i) R[i] = Offset[Parent[i]];
    else R[i] = Offset[Parent[i]] + vertexWeights[M[i]];
  }
  return R;
}


intT* Matching(weightedArcArray EE, intT* vertexWeights) {
  weightedArc* E = EE.E;
  intT n = EE.n;
  intT m = EE.m;
  intT* T = newA(intT,n);
  float* reservations = newA(float,n);

  weightTime.start();

  parallel_for(intT i=0;i<n;i++) { reservations[i] = -1; T[i] = -1; }

  float* ratios = newA(float,m);
  parallel_for(intT i=0;i<m;i++) {
    intT u = E[i].u();
    intT v = E[i].v();
    //add some small amount to ratio to break ties deterministically (given edge ordering)
    //float noise = float(i)/(m*m);
    ratios[i] = 
      float(E[i].weight)/(float(vertexWeights[u])*float(vertexWeights[v]));
  }
  weightTime.stop();
  reservationTime.start();
  //for(int i=0;i<m;i++)cout<<ratios[i]<<" ";cout<<endl;
  //reserve
  parallel_for(intT i=0;i<m;i++) {
    intT u = E[i].u();
    intT v = E[i].v();
    utils::writeMax(&reservations[u],ratios[i]);
    utils::writeMax(&reservations[v],ratios[i]);
  }
  reservationTime.stop();
  //for(int i=0;i<n;i++)cout<<reservations[i]<<" ";cout<<endl;

  //commit
  commitTime.start();
  parallel_for(intT i=0;i<m;i++){
    intT u = E[i].u();
    intT v = E[i].v();
    if(reservations[u] == ratios[i]) T[u] = v;
    if(reservations[v] == ratios[i]) T[v] = u;
  }
  commitTime.stop();
  //cout<<"T:";for(int i=0;i<n;i++)cout<<T[i]<<" ";cout<<endl;

  free(reservations); free(ratios);

  // // Generate a maximal matching on the links in T
  matchingTime.start();
  intT* M = maximalMatchingT(T,n);
  matchingTime.stop();
  free(T);
  //for(int i=0;i<n;i++)cout<<M[i]<<" ";cout<<endl;

  return M;
}  

// The main separator loop
// It switches back and forth between two arrays for holding the edges.
intT* separatorR(intT* vertexWeights, weightedArcArray E, weightedArcArray EO, intT round) {
  intT n = E.n;
  intT m = E.m;
  cout << "Separator: size = " << n << " round = " << round << endl;
  if (n == 1 || m == 0) {
    intT* R = newA(intT,n); 
    intT count=0;
    for (intT i=0; i < n; i++) {R[i] = count; count+=vertexWeights[i];}
    cout << "Separator rounds = " << round << " n="<<n<<" m="<<m<<endl;
    return R;
  } else if (round > 200) {
    cout << "Too many rounds in Separator" << endl;
    abort();
  } else {
    double t1, t2;

    // find a matching based on edge and vertex weights
    intT* M = Matching(E,vertexWeights);
    // // array of parent pointers for contractGraph
    intT* Parent = newA(intT,n);
    //cout<<"M:round "<<round<<" ;";for(int i=0;i<n;i++)cout<<M[i]<<" ";cout<<endl;
    //cout<<"vertexWeights: ";for(int i=0;i<n;i++)cout<<vertexWeights[i]<<" ";cout<<endl;
    //cout<<"E: ";for(int i=0;i<m;i++)cout<<"("<<E.E[i].u()<<","<<E.E[i].v()<<","<<E.E[i].weight<<") ";cout<<endl;

    
    intT* vWeights = contractGraph(M, Parent, E, EO, vertexWeights);

    //cout<<"Parent:round "<<round<<" ;";for(int i=0;i<n;i++)cout<<Parent[i]<<" ";cout<<endl;

    // // recurse on contracted graph
    intT* R = separatorR(vWeights, EO, E, round+1);
    //cout<<"n="<<n<<" m="<<m<<endl;
    //cout<<"M:round "<<round<<" ;";for(int i=0;i<n;i++)cout<<M[i]<<" ";cout<<endl;
    //cout<<"Parent:round "<<round<<" ;";for(int i=0;i<n;i++)cout<<Parent[i]<<" ";cout<<endl;
    //cout<<"R:round "<<round<<" ;";for(int i=0;i<n;i++)cout<<R[i]<<" ";cout<<endl;
    //cout<<"vertexWeights:";for(int i=0;i<n;i++)cout<<vertexWeights[i]<<" ";cout<<endl;
    // uncontract and generate ordering in RR
    
    intT* RR = uncontractGraph(n, vertexWeights, R, M, Parent);
    //cout<<"RR:round"<<round<<" ";for(int i=0;i<n;i++)cout<<RR[i]<<" ";cout<<endl;

    free(Parent); free(M); free(vWeights);
    return RR;
  }
}


intT* separator(edgeArray<intT> G) {
  intT n = max<intT>(G.numCols,G.numRows);
  intT m = G.nonZeros;
  cout<<"n="<<n<<" m="<<m<<endl;
  initTime.start();
  weightedArc* E = newA(weightedArc,m);
  weightedArc* EO = newA(weightedArc,m);
  intT* vertexWeights = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) vertexWeights[i] = 1;
  parallel_for(intT i=0;i<m;i++) { 
    if(G.E[i].u < G.E[i].v) E[i] = weightedArc(G.E[i].u,G.E[i].v,1);
    else E[i] = weightedArc(G.E[i].v,G.E[i].u,1);
  }
  initTime.stop();

  intT *R = separatorR(vertexWeights, weightedArcArray(E,n,m), weightedArcArray(EO,n,m), (intT)0);
  free(E); free(EO); free(vertexWeights);
  if (DETAILED_TIME) reportDetailedTime();

  return R;
}

typedef pair<double,intT> diPair;

struct edgeLessPair{
  bool operator() (diPair a, diPair b){
    return (a.second < b.second); }
};


struct addDiPair { diPair operator() (diPair a, diPair b){
  b.first=a.first+b.first;return b;
  }
};

struct minDiPair { diPair operator() (diPair a, diPair b){
  return (a.first < b.first) ? a : b;
}
};

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

// Make sure the ordering is proper
void checkOrder(intT* I, intT n) {
  intT* II = newA(intT,n);
  for (intT i=0; i < n; i++) II[i] = I[i];
  sort(II,II+n);
  for (intT i=0; i < n; i++)
    if (II[i] != i) {
      cout << "Bad ordering at i = " << i << " val = " << II[i] << endl;
      break;
    }
  free(II);
}

void test(edgeArray<intT> G) {


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
  intT* I = separator(G);
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



