#include <iostream>
#include <limits>
#include "seq.h"
#include "cilk.h"
//#include "quadOctTree.h"
#include "ppOctTree.h"
using namespace std;

// should be a variable, but alloction is too expensive
//#define maxK 10

//int inter = 0;

// A k-nearest neighbor structure
// requires vertexT to have pointT and vectT typedefs
template <class vertexT, int maxK>
struct kNearestNeighbor {
  typedef vertexT vertex;
  typedef typename vertexT::pointT point;
  typedef typename point::vectT fvect;

  typedef gTreeNode<point,fvect,vertex> qoTree;
  qoTree *tree;

  // generates the search structure
  kNearestNeighbor(seq<vertex*> vertices) {
    tree = qoTree::gTree(vertices);
  }

  // returns the vertices in the search structure, in an
  //  order that has spacial locality
  seq<vertex*> vertices() {
    return tree->flatten();
  }

  void del() {tree->del();}

  struct kNN {
    vertex *ps;  // the element for which we are trying to find a NN
    vertex *pn[maxK];  // the current k nearest neighbors (nearest last)
    double rn[maxK]; // radius of current k nearest neighbors
    int quads;
    int k;
    kNN() {}

    // returns the ith smallest element (0 is smallest) up to k-1
    vertex* operator[] (const int i) { return pn[k-i-1]; }

    kNN(vertex *p, int kk) {
      if (kk > maxK) {cout << "k too large in kNN" << endl; abort();}
      k = kk;
      quads = (1 << (p->pt).dimension());
      ps = p;
      //pn = newA(vertex*,k); // too expensive in parallel
      //rn = newA(double,k);
      for (int i=0; i<k; i++) {
	pn[i] = (vertex*) NULL; 
	rn[i] = numeric_limits<double>::max();
      }
    }

    // if p is closer than pn then swap it in
    void update(vertex *p) { 
      //inter++;
      point opt = (p->pt);
      fvect v = (ps->pt) - opt;
      double r = v.Length();
      if (r < rn[0]) {
	pn[0]=p; rn[0] = r;
	for (int i=1; i < k && rn[i-1]<rn[i]; i++) {
	  swap(rn[i-1],rn[i]); swap(pn[i-1],pn[i]); }
      }
    }

    // looks for nearest neighbors in boxes for which ps is not in
    void nearestNghTrim(qoTree *T) {
      if (!(T->center).outOfBox(ps->pt, (T->size/2)+rn[0]))
	if (T->IsLeaf())
	  for (int i = 0; i < T->count; i++) update(T->vertices[i]);
	else 
	  for (int j=0; j < quads; j++) nearestNghTrim(T->children[j]);
    }

    // looks for nearest neighbors in box for which ps is in
    void nearestNgh(qoTree *T) {
      if (T->IsLeaf())
	for (int i = 0; i < T->count; i++) {
	  vertex *pb = T->vertices[i];
	  if (pb != ps) update(pb);
	}
      else {
	int i = T->findQuadrant(ps);
	nearestNgh(T->children[i]);
	for (int j=0; j < quads; j++) 
	  if (j != i) nearestNghTrim(T->children[j]);
      }
    }
  };

  vertex* nearest(vertex *p) {
    kNN nn(p,1);
    nn.nearestNgh(tree); 
    return nn[0];
  }

  // version that writes into result
  void kNearest(vertex *p, vertex** result, int k) {
    kNN nn(p,k);
    nn.nearestNgh(tree); 
    for (int i=0; i < k; i++) result[i] = 0;
    for (int i=0; i < k; i++) result[i] = nn[i];
  }

  // version that allocates result
  vertex** kNearest(vertex *p, int k) {
    vertex** result = newA(vertex*,k);
    kNearest(p,result,k);
    return result;
  }

};

// find the k nearest neighbors for all points in tree
// places pointers to them in the .ngh field of each vertex
template <int maxK, class vertexT>
void ANN(vertexT** v, int n, int k) {
  typedef kNearestNeighbor<vertexT,maxK> kNNT;

  //startTime();
  kNNT T = kNNT(seq<vertexT*>(v,n));
  //nextTime("build tree");

  // this reorders the vertices for locality
  seq<vertexT*> vr = T.vertices();

  // find nearest k neighbors for each point
  _cilk_grainsize_2
  cilk_for(int i=0; i < n; i++) 
    T.kNearest(vr[i],vr[i]->ngh,k);

  vr.del();
  T.del();
}
