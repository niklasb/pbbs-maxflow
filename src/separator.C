#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "limits.h"
#include "gettime.h"
#include "graph.h"
#include "cilk.h"
#include "quickSort.h"
using namespace std;

vindex* maximalMatchingT(seq<vindex>);
void checkMaximalMatchingT(seq<vindex>, vindex* GD);

// **************************************************************
//    SOME TIMING CODE
// **************************************************************

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
  vindex ngh;
  int weight;
  weightedArc(vindex n, int w) : ngh(n), weight(w) {}
};

struct weightedVertex {
  weightedArc* Neighbors;
  int degree;
  int weight;
  void del() {free(Neighbors);}
  weightedVertex(weightedArc* N, int d, int w) 
    : Neighbors(N), degree(d), weight(w) {}
};

// A graph with integer edge and vertex weights
struct weightedGraph {
  weightedVertex *V;
  int n;
  int m;
  weightedGraph(weightedVertex* VV, int nn, int mm) 
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
			    vindex* M, vindex* Parent, weightedArc* E) {

  weightedVertex* V = G.V;  //int* W = G.W;
  vindex* Degree = newA(vindex,G.n);

  // reserve space for children pointers and mark ones to keep
  cilk_for (int i=0; i < G.n; i++) {
    if (M[i] < 0) {
      Degree[i] = V[i].degree;
      Parent[i] = 1;
    }
    else if (M[i] < i) {
      Degree[i] = V[i].degree+V[M[i]].degree;
      Parent[i] = 1;
    }
    else Degree[i] = Parent[i] = 0;
  }

  int space = sequence::scan(Degree,Degree,G.n,utils::addF<int>(),0);
  int count = sequence::scan(Parent,Parent,G.n,utils::addF<int>(),0);
  weightedVertex* VO = newA(weightedVertex,count);

  cilk_for (int i=0; i < G.n; i++) 
    if (M[i] > i) Parent[i] = Parent[M[i]];

  cilk_for (int i=0; i < G.n; i++) {
    // Either leader or unmatched
    int k = 0;
    vindex parent = Parent[i];
    if (M[i] < i) {
      weightedArc *EO = E + Degree[i];
      for (int j=0; j<V[i].degree; j++) {
	weightedArc u = V[i].Neighbors[j];
	vindex p = Parent[u.ngh];
	if (p != parent) EO[k++] = weightedArc(p,u.weight);
      }
      int weight = V[i].weight;
      if (M[i] >= 0) {
	int ii = M[i];
	for (int j=0; j<V[ii].degree; j++) {
	  weightedArc u = V[ii].Neighbors[j];
	  vindex p = Parent[u.ngh];
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
void foo(int n, int* R, weightedVertex* V,
	int* Offset, vindex* M, vindex* Parent) {
  cilk_for(int i = 0; i < n; i++) {
    if (M[i] < i) R[i] = Offset[Parent[i]];
    //else if (M[i] < i) R[i] = Offset[Parent[i]];
    else R[i] = Offset[Parent[i]] + V[M[i]].weight;
  }
}

// Undoes the contraction done by contract Graph.
// In particular given the array of integers Offset for the contracted graph
//  it gives the left child of location i, Offset[i]
//  and the right child, Offset[i]+ W[LeftChild[i]]
// Basically a parallel prefix operation over the contraction tree
int* uncontractGraph(weightedGraph G, int* Offset, vindex* M, vindex* Parent) {
  int* R = newA(int,G.n);
  // Cilk++ is broken so the cilk loop had to be lifted out -- who knows why
  foo(G.n, R, G.V, Offset, M, Parent);
  return R;
}

// another lifted loop
void bar(vindex* M, int n) {
  cilk_for (int ii = 0; ii < n; ii++) { if (M[ii] == INT_MAX) M[ii] = -1; }
}

inline int hashVertex(int round, vindex i) {
  return int ((((unsigned int) -1) >> 1) & utils::hash(round + i));
}


vindex* Matching(weightedGraph G) {
  int n = G.n;
  weightedVertex* V = G.V;
  vindex* T = newA(vindex,n);
  double t1, t2;


  weightTime.start();
  cilk_for (int i = 0; i < n; i++) {
    if (V[i].degree == 0) T[i] = -1;
    else {
      // remove duplicate edges
      int k = 1;
      //vindex prev = -1;
      for (int j=1; j < V[i].degree; j++)
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
      int maxNgh = -1;

      // start at psuedorandom edge to "fairly" break ties
      int jj = hashVertex(0,i)%V[i].degree;
      for (int j=0; j < V[i].degree; j++, jj++) {
	if (jj == V[i].degree) jj = 0;
	vindex ngh = V[i].Neighbors[jj].ngh;
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
  seq<vindex> TT = seq<vindex>(T,n);
  matchingTime.start();
  vindex* M = maximalMatchingT(TT);
  matchingTime.stop();

  //checkMaximalMatchingT(TT,M); //remove later
  TT.del();
  bar(M,n);
  return M;
}  

// The main separator loop
// It switches back and forth between two arrays for holding the edges.
int* separatorR(weightedGraph G, weightedArc* E, weightedArc* EO, int round) {
  //cout << "Separator: size = " << G.n << " round = " << round << endl;
  if (G.n == 1 || G.m == 0) {
    int* R = newA(int,G.n); 
    int count=0;
    for (int i=0; i < G.n; i++) {R[i] = count; count+=G.V[i].weight;}
    //cout << "Separator rounds = " << round << endl;
    return R;
  } else if (round > 200) {
    cout << "Too many rounds in Separator" << endl;
    abort();
  } else {
    double t1, t2;
    int n = G.n;

    // find a matching based on edge and vertex weights
    vindex* M = Matching(G);

    // array of parent pointers for contractGraph
    vindex* Parent = newA(vindex,G.n);

    contractTime.start();
    weightedGraph GN = contractGraph(G, M, Parent, EO);
    contractTime.stop();
    // recurse on contracted graph
    int* R = separatorR(GN, EO, E, round+1);
    //cout<<"M:round"<<round<<" ";for(int i=0;i<n;i++)cout<<M[i]<<" ";cout<<endl;
    //cout<<"Parent:round"<<round<<" ";for(int i=0;i<n;i++)cout<<Parent[i]<<" ";cout<<endl;

    //cout<<"R:round"<<round<<" ";for(int i=0;i<n;i++)cout<<R[i]<<" ";cout<<endl;
    // uncontract and generate ordering in RR
    int* RR = uncontractGraph(G, R, M, Parent);
    //cout<<"RR:round"<<round<<" ";for(int i=0;i<n;i++)cout<<RR[i]<<" ";cout<<endl;

    free(Parent);  free(M);  GN.del();
    return RR;
  }
}

int* separator(graph G) {
  // the temporary arrays for holding edges during contraction
  weightedArc* E = newA(weightedArc,G.m);
  weightedArc* EO = newA(weightedArc,G.m);
  weightedVertex* V = newA(weightedVertex,G.n);
  vindex* Degree = newA(vindex,G.n);

  // reserve space to copy edges into E
  cilk_for (int i=0; i < G.n; i++) Degree[i] = G.V[i].degree;
  int m = sequence::scan(Degree,Degree,G.n,utils::addF<int>(),0);

  // create weighted graph
  // self-edges are removed as copied into weighted version
  int edgeWeight = 1;
  int vertexWeight = 1;
  cilk_for (int i=0; i < G.n; i++) {
    weightedArc* EE = E + Degree[i];
    int k = 0;
    for (int j=0; j < G.V[i].degree; j++) {
      vindex ngh = G.V[i].Neighbors[j];
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
  int *R = separatorR(weightedGraph(V,G.n,m),E, EO, 0);
  free(E); free(EO); free(V);
  if (DETAILED_TIME) reportDetailedTime();
  //for(int i=0;i<G.n;i++)cout<<R[i]<<" ";cout<<endl;
  return R;
}
