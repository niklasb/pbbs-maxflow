#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "cilk.h"
#include "geom.h"
#include "intSort.h"
using namespace std;

// *************************************************************
//    QUAD/OCT TREE NODES
// *************************************************************

#define gMaxLeafSize 16  // number of points stored in each leaf

template <class vertex>
struct nData {
  int cnt;
  nData(int x) : cnt(x) {}
  nData() : cnt(0) {}
  nData operator+(nData op2) {return nData(cnt+op2.cnt);}
  nData operator+(vertex* op2) {return nData(cnt+1);}
};

template <class pointT, class vectT, class vertexT, class nodeData = nData<vertexT> >
class gTreeNode {
private :

public :
  typedef pointT point;
  typedef vectT fvect;
  typedef vertexT vertex;
  struct getPoint {point operator() (vertex* v) {return v->pt;}};
  struct minpt {point operator() (point a, point b) {return a.minCoords(b);}};
  struct maxpt {point operator() (point a, point b) {return a.maxCoords(b);}};

  point center; // center of the box
  double size;   // width of each dimension, which have to be the same
  nodeData data; // total mass of vertices in the box
  int count;  // number of vertices in the box
  gTreeNode *children[8];
  vertex **vertices;

  // wraps a bounding box around the points and generates
  // a tree.
  static gTreeNode* gTree(seq<vertex*> vv) {
    seq<vertex*> v = vv.copy();
    point* pt = newA(point,v.size());
    cilk_for(int i=0; i < v.size(); i++) pt[i] = v[i]->pt;
    seq<point> pts = seq<point>(pt,v.size());
    point minPt = pts.reduce(minpt());
    point maxPt = pts.reduce(maxpt());
    free(pt);
    //cout << "min "; minPt.print(); cout << endl;
    //cout << "max "; maxPt.print(); cout << endl;
    fvect box = maxPt-minPt;
    point center = minPt+(box/2.0);
    gTreeNode* result = new gTreeNode(v,center,box.maxDim());
    v.del();
    return result;
  }

  int IsLeaf() { return (vertices != NULL); }

  void del() {
    if (IsLeaf()) delete [] vertices;
    else {
      for (int i=0 ; i < (1 << center.dimension()); i++) {
	children[i]->del();
	delete children[i];
      }
    }
  }

  // Returns the depth of the tree rooted at this node
  int Depth() {
    int depth;
    if (IsLeaf()) depth = 0;
    else {
      depth = 0;
      for (int i=0 ; i < (1 << center.dimension()); i++)
	depth = max(depth,children[i]->Depth());
    }
    return depth+1;
  }

  // Returns the size of the tree rooted at this node
  int Size() {
    int sz;
    if (IsLeaf()) {
      sz = count;
      for (int i=0; i < count; i++) 
	if (vertices[i] < ((vertex*) NULL)+1000) 
	  cout << "oops: " << vertices[i] << "," << count << "," << i << endl;
    }
    else {
      sz = 0;
      for (int i=0 ; i < (1 << center.dimension()); i++)
	sz += children[i]->Size();
    }
    return sz;
  }

  template <class F>
  void applyIndex(int s, F f) {
    if (IsLeaf())
      for (int i=0; i < count; i++) f(vertices[i],s+i);
    else {
      int ss = s;
      for (int i=0 ; i < (1 << center.dimension()); i++) {
	cilk_spawn children[i]->applyIndex(ss,f);
	ss += children[i]->count;
      }
      cilk_sync;
    }
  }

  struct flatten_FA {
    vertex** _A;
    flatten_FA(vertex** A) : _A(A) {}
    void operator() (vertex* p, int i) {_A[i] = p;}
  };

  seq<vertex*> flatten() {
    vertex** A = new vertex*[count];
    applyIndex(0,flatten_FA(A));
    return seq<vertex*>(A,count);
  }

  // Returns the child the vertex p appears in
  int findQuadrant (vertex* p) {
    return (p->pt).quadrant(center); }

  // A hack to get around Cilk shortcomings
  static gTreeNode *newTree(seq<vertex*> S, point cnt, double sz) {
    return new gTreeNode(S,cnt,sz); }

  // Used as a closure in collectD
  struct findChild {
    gTreeNode *tr;
    findChild(gTreeNode *t) : tr(t) {}
    int operator() (vertex* p) {
      int r = tr->findQuadrant(p);
      return r;}
  };

  gTreeNode(seq<vertex*> S, point cnt, double sz)
  {
    count = S.size();
    size = sz;
    center = cnt;
    vertices = NULL;
    int quadrants = (1 << center.dimension());

    if (count > gMaxLeafSize) {
      int offsets[8];
      intSort::iSort(S.S,offsets,S.sz,quadrants,findChild(this));
      // Give each child its appropriate center and size
      // The centers are offset by size/4 in each of the dimensions
      for (int i=0 ; i < quadrants; i++) {
	int l = ((i == quadrants-1) ? S.sz : offsets[i+1]) - offsets[i];
	seq<vertex*> A = seq<vertex*>(S.S+offsets[i],l);
	point newcenter = center.offsetPoint(i, size/4.0);
	children[i] = cilk_spawn newTree(A,newcenter,size/2.0);
      }
      cilk_sync;
      for (int i=0 ; i < quadrants; i++) 
	data = data + children[i]->data;
    } else {
      vertices = new vertex*[count];
      for (int i=0; i < count; i++) {
	vertex* p = S[i];
	data = data + p;
	vertices[i] = p;
      }
      //S.del();
    }
  }
};
