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
#ifdef OPENMP
#include "stlParallelSort.h"
#else
#include "quickSort.h"
#endif
#include "maxMatching.h"
#include "graphUtils.h"
using namespace std;

#define DETAILED_TIME 1

timer matchingTime;
timer contractTime;
timer sortTime;
timer weightTime;

void reportDetailedTime() {
  //sortTime.reportTotal("  sorting time");
  matchingTime.reportTotal("  matching time");
  contractTime.reportTotal("  contract time");
  weightTime.reportTotal("  weight time");
}

// **************************************************************
//    SEPARATOR
// **************************************************************

// In a weighted graph the vertex weight represents the number of nodes
// in the contracted component, and the edge weight represents the number
// of original edges between two components.

struct weightedArc {
  uintT ngh;
  uintT weight;
  weightedArc(uintT n, uintT w) : ngh(n), weight(w) {}
};

struct weightedVertex {
  weightedArc* Neighbors;
  uintT degree;
  uintT weight;
  void del() {free(Neighbors);}
  weightedVertex(weightedArc* N, uintT d, uintT w) 
    : Neighbors(N), degree(d), weight(w) {}
};

// A graph with integer edge and vertex weights
struct weightedGraph {
  weightedVertex *V;
  uintT n;
  uintT m;
  weightedGraph(weightedVertex* VV, uintT nn, uintT mm) 
    : V(VV), n(nn), m(mm) {}
  void del() { free(V);}
};

// used for sorting the edges
struct edgeLess : std::binary_function <weightedArc,weightedArc,bool> {
  bool operator() (weightedArc const& a, weightedArc const& b) const
  { return (a.ngh < b.ngh); }
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
// E is an array used for the new edges (arcs).   It must be at least as 
//   long as the number of edges (arcs) in G.
// This routine returns a new contracted graph where the arcs are
//   allocated from E.
weightedGraph contractGraph(weightedGraph G, 
			    uintT* M, uintT* Parent, weightedArc* E) {

  weightedVertex* V = G.V;  //int* W = G.W;
  uintT* Degree = newA(uintT,G.n);

  // reserve space for children pointers and mark ones to keep
  parallel_for (uintT i=0; i < G.n; i++) {
    if (M[i] == UINT_T_MAX) {
      Degree[i] = V[i].degree;
      Parent[i] = 1;
    }
    else if (M[i] < i) {
      Degree[i] = V[i].degree+V[M[i]].degree;
      Parent[i] = 1;
    }
    else Degree[i] = Parent[i] = 0;
  }

  uintT space = sequence::scan(Degree,Degree,G.n,utils::addF<uintT>(),(uintT)0);
  uintT count = sequence::scan(Parent,Parent,G.n,utils::addF<uintT>(),(uintT)0);
  weightedVertex* VO = newA(weightedVertex,count);

  parallel_for (uintT i=0; i < G.n; i++) 
    if (M[i] != UINT_T_MAX && M[i] > i) Parent[i] = Parent[M[i]];

  parallel_for (uintT i=0; i < G.n; i++) {
    // Either leader or unmatched
    uintT k = 0;
    uintT parent = Parent[i];
    if (M[i] == UINT_T_MAX || M[i] < i) {
      weightedArc *EO = E + Degree[i];
      for (uintT j=0; j<V[i].degree; j++) {
	weightedArc u = V[i].Neighbors[j];
	uintT p = Parent[u.ngh];
	if (p != parent) EO[k++] = weightedArc(p,u.weight);
      }
      uintT weight = V[i].weight;
      if (M[i] != UINT_T_MAX) {
	uintT ii = M[i];
	for (uintT j=0; j<V[ii].degree; j++) {
	  weightedArc u = V[ii].Neighbors[j];
	  uintT p = Parent[u.ngh];
	  if (p != parent) EO[k++] = weightedArc(p,u.weight);
	}
	weight += V[ii].weight;
      }
      VO[parent] = weightedVertex(EO,k,weight);
      compSort(EO,k,edgeLess());
    }
  }
  return weightedGraph(VO,count,space);
}

// A hack since Cilk++ is broken.   Putting this in the code below
// crashes and burns with "gcc version 4.2.4 (Cilk Arts build 8503)"
void foo(uintT n, uintT* R, weightedVertex* V,
	uintT* Offset, uintT* M, uintT* Parent) {
  parallel_for(uintT i = 0; i < n; i++) {
    if (M[i] == UINT_T_MAX || M[i] < i) R[i] = Offset[Parent[i]];
    //else if (M[i] < i) R[i] = Offset[Parent[i]];
    else R[i] = Offset[Parent[i]] + V[M[i]].weight;
  }
}

// Undoes the contraction done by contract Graph.
// In particular given the array of integers Offset for the contracted graph
//  it gives the left child of location i, Offset[i]
//  and the right child, Offset[i]+ W[LeftChild[i]]
// Basically a parallel prefix operation over the contraction tree
uintT* uncontractGraph(weightedGraph G, uintT* Offset, uintT* M, uintT* Parent) {
  uintT* R = newA(uintT,G.n);
  // Cilk++ is broken so the cilk loop had to be lifted out -- who knows why
  foo(G.n, R, G.V, Offset, M, Parent);
  return R;
}

// another lifted loop
// void bar(uintT* M, uintT n) {
//   parallel_for (uintT ii = 0; ii < n; ii++) { 
//     if (M[ii] == UINT_T_MAX) M[ii] = -1; }
// }

inline uintT hashVertex(uintT round, uintT i) {
  return uintT ((((unsigned int) -1) >> 1) & utils::hash(round + i));
}


uintT* Matching(weightedGraph G) {
  uintT n = G.n;
  weightedVertex* V = G.V;
  uintT* T = newA(uintT,n);
  double t1, t2;


  weightTime.start();
  parallel_for (uintT i = 0; i < n; i++) {
    if (V[i].degree == 0) T[i] = UINT_T_MAX;
    else {
      // remove duplicate edges
      uintT k = 1;
      //intT prev = -1;
      for (uintT j=1; j < V[i].degree; j++)
	if (V[i].Neighbors[j].ngh != V[i].Neighbors[k-1].ngh)
	  V[i].Neighbors[k++] = V[i].Neighbors[j];
	else
	  V[i].Neighbors[k-1].weight += V[i].Neighbors[j].weight;
      V[i].degree = k;

      // Pick outgoing edge with maximum ratio of the edge weigh 
      //   to the weight of the neighbor.   In particular the ratio
      //   of the number of original edges to the neighboring component
      //   to the number of original vertices in the neighbor.
      // This biases towards linking to light and tightly-connected
      //    neighbors 
      float maxWeight = 0.0;
      uintT maxNgh = UINT_T_MAX;

      // start at psuedorandom edge to "fairly" break ties
      uintT jj = hashVertex(0,i)%V[i].degree;
      for (uintT j=0; j < V[i].degree; j++, jj++) {
	if (jj == V[i].degree) jj = 0;
	uintT ngh = V[i].Neighbors[jj].ngh;
	float weight = float(V[i].Neighbors[jj].weight)/float(V[ngh].weight);
	if (weight > maxWeight) {
	  maxWeight = weight;
	  maxNgh = ngh;
	}
      }
      T[i] = maxNgh;
    }
  }
  weightTime.stop();

  // Generate a maximal matching on the links in T
  //seq<intT> TT = seq<intT>(T,n);
  matchingTime.start();
  uintT* M = maximalMatchingT(T,n);
  matchingTime.stop();

  //checkMaximalMatchingT(TT,M); //remove later
  //TT.del();
  free(T);
  //bar(M,n);
  return M;
}  

// The main separator loop
// It switches back and forth between two arrays for holding the edges.
uintT* separatorR(weightedGraph G, weightedArc* E, weightedArc* EO, uintT round) {
  cout << "Separator: size = " << G.n << " round = " << round << endl;
  if (G.n == 1 || G.m == 0) {
    uintT* R = newA(uintT,G.n); 
    uintT count=0;
    for (uintT i=0; i < G.n; i++) {R[i] = count; count+=G.V[i].weight;}
    //cout << "Separator rounds = " << round << endl;
    return R;
  } else if (round > 2000) {
    cout << "Too many rounds in Separator" << endl;
    abort();
  } else {
    double t1, t2;
    uintT n = G.n;

    // find a matching based on edge and vertex weights
    uintT* M = Matching(G);

    // array of parent pointers for contractGraph
    uintT* Parent = newA(uintT,G.n);

    contractTime.start();
    weightedGraph GN = contractGraph(G, M, Parent, EO);
    contractTime.stop();
    // recurse on contracted graph
    uintT* R = separatorR(GN, EO, E, round+1);
    //cout<<"M:round"<<round<<" ";for(int i=0;i<n;i++)cout<<M[i]<<" ";cout<<endl;
    //cout<<"Parent:round"<<round<<" ";for(int i=0;i<n;i++)cout<<Parent[i]<<" ";cout<<endl;

    //cout<<"R:round"<<round<<" ";for(int i=0;i<n;i++)cout<<R[i]<<" ";cout<<endl;
    // uncontract and generate ordering in RR
    uintT* RR = uncontractGraph(G, R, M, Parent);
    //cout<<"RR:round"<<round<<" ";for(int i=0;i<n;i++)cout<<RR[i]<<" ";cout<<endl;

    free(Parent);  free(M);  GN.del();
    return RR;
  }
}

uintT* separator(graph<uintT> G) {
  // the temporary arrays for holding edges during contraction
  weightedArc* E = newA(weightedArc,G.m);
  weightedArc* EO = newA(weightedArc,G.m);
  weightedVertex* V = newA(weightedVertex,G.n);
  uintT* Degree = newA(uintT,G.n);

  // reserve space to copy edges into E
  parallel_for (uintT i=0; i < G.n; i++) Degree[i] = G.V[i].degree;
  uintT m = sequence::scan(Degree,Degree,G.n,utils::addF<uintT>(),(uintT)0);

  // create weighted graph
  // self-edges are removed as copied into weighted version
  uintT edgeWeight = 1;
  uintT vertexWeight = 1;
  parallel_for (uintT i=0; i < G.n; i++) {
    weightedArc* EE = E + Degree[i];
    uintT k = 0;
    for (uintT j=0; j < G.V[i].degree; j++) {
      uintT ngh = G.V[i].Neighbors[j];
      if (ngh != i) {
	EE[k].ngh = G.V[i].Neighbors[j];
	EE[k].weight = edgeWeight;
	k++;
      }
    }
    V[i] = weightedVertex(EE,k,vertexWeight);
  }
  free(Degree);

  // and Go
  uintT *R = separatorR(weightedGraph(V,G.n,m),E, EO, (uintT)0);
  free(E); free(EO); free(V);
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

intT bestCut(graph<uintT> G){
  diPair* edges = newA(diPair,2*G.m);
  intT* offsets = newA(intT,G.n);
  
  parallel_for(intT i=0;i<G.n;i++){
    offsets[i] = G.V[i].degree<<1;
  }

  sequence::scan(offsets,offsets,G.n,utils::addF<intT>(),(intT)0);

  parallel_for(intT i=0;i<G.n;i++){
    intT o = offsets[i];
    for(intT j=0;j<G.V[i].degree;j++){
      intT jj = j<<1;
      if(G.V[i].Neighbors[j] > i){
	edges[o+jj].first = 1;
	edges[o+jj+1].first = -1;
      } else {
	edges[o+jj].first = 0;
	edges[o+jj+1].first = 0;
      }
      edges[o+jj].second = i;
      edges[o+jj+1].second = G.V[i].Neighbors[j];
      
    }
  }
 
  intSort::iSort(edges,2*G.m,G.n,utils::secondF<double,intT>());
  
  sequence::scan(edges,edges,2*G.m,addDiPair(),make_pair(0.0,(intT)-1));

  //get max at offset points
  diPair* cuts = newA(diPair,G.n-1);
  parallel_for(intT i=1;i<G.n;i++){
    intT ii=i-1;
    cuts[ii] = edges[offsets[i]];
    cuts[ii].first /= i;
    cuts[ii].first /= G.n-i;
  }
  free(offsets);
  diPair best = sequence::reduce(cuts,G.n-1,minDiPair());

  cout<<"Sparsest cut position: cut after position " << best.second
      << " Cost: " << best.first << endl;

  free(cuts); free(edges);
  return best.second;
}

// number of off diagonals for diagonal blocks of size blockSize
double sepSize(graph<uintT> G, uintT blockSize) {
  double k = 0;
  for (uintT i = 0; i < G.n; i++)
    for (uintT j = 0; j < G.V[i].degree; j++) 
      if ((i / blockSize) != (G.V[i].Neighbors[j] / blockSize)) k+= 1;
  return k;
}

double logCost(graph<uintT> G) {
  double logs = 0.0;
  for (intT i = 0; i < G.n; i++) { 
    for (intT j = 0; j < G.V[i].degree; j++) {
      double logcost = log(((double) abs(i - (intT)G.V[i].Neighbors[j])) + 1);
      logs += logcost;
    }
  }
  return logs/(G.m*log(2.0));
}

// Make sure the ordering is proper
void checkOurOrder(uintT* I, uintT n) {
  uintT* II = newA(uintT,n);
  for (uintT i=0; i < n; i++) II[i] = I[i];
  sort(II,II+n);
  for (uintT i=0; i < n; i++)
    if (II[i] != i) {
      cout << "Bad ordering at i = " << i << " val = " << II[i] << endl;
      break;
    }
  free(II);
}

void test(graph<uintT> G) {
  // double s8 = sepSize(G, 1<<8);
  // double s16 = sepSize(G, 1<<16);
  // double lc = logCost(G); 
  //graphCheckConsistency(G);
  //randomly reorder
  //G = graphReorder(G,(intT*)NULL);


  uintT* I = separator(G);
  //stopTime(weight,str);


  // Check correctness
  checkOurOrder(I, G.n);
  cout << "checked order\n";

  // Check block separator sizes with reordering
  // graph<uintT> GG = graphReorder(G,I);
  // cout << "reordered\n";
  // graphCheckConsistency(GG);
  // cout << "checked graph consistency\n";

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



