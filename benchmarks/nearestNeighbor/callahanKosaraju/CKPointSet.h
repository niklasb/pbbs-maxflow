/*
 *  CKPointSet.h
 *  CallahanKosaraju
 *
 *  Created by Aapo Kyrola on 11/9/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#ifndef __DEF_CKPOINTSET_
#define __DEF_CKPOINTSET_

#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <cmath>

#define DIMENSIONS 3
#define BOXEDGES 8
#define MAXPARALLELDEPTH 5
#define _sqr(x) ((x)*(x))

class treenode;

struct point {
  double coord[DIMENSIONS]; // position
  double m;
  int nn;
  treenode * node;
  
  point() { nn = (-1); } 
  point(double x, double y, double z) {
    coord[0] = x;
    coord[1] = y;
    coord[2] = z;    
    nn = (-1); 
  }
  
  
  double rsqr(point &p) {
    assert(DIMENSIONS == 3);
    return _sqr(p.coord[0]-coord[0])+_sqr(p.coord[1]-coord[1])+_sqr(p.coord[2]-coord[2]);
  } 
  
};


struct staticpoint {
  double coord[DIMENSIONS];
  
  double rsqr(point &p) {
    assert(DIMENSIONS==3);
    return _sqr(p.coord[0]-coord[0])+_sqr(p.coord[1]-coord[1])+_sqr(p.coord[2]-coord[2]);
  }
  double rsqr(staticpoint &p) {
    assert(DIMENSIONS==3);
    return _sqr(p.coord[0]-coord[0])+_sqr(p.coord[1]-coord[1])+_sqr(p.coord[2]-coord[2]);
  }
  staticpoint() {}
  staticpoint(double x, double y, double z) {
    coord[0] = x;
    coord[1] = y;
    coord[2] = z;    
  }
};

struct box {
  staticpoint edges[BOXEDGES];
  staticpoint center;
  double r;
  box() {}
  box(double x, double xdim, double y, double ydim, double z, double zdim) {
    int k =0;
    edges[k++] = staticpoint(x, y, z);
    edges[k++] = staticpoint(x+xdim, y, z);
    edges[k++] = staticpoint(x+xdim, y+ydim, z);
    edges[k++] = staticpoint(x, y+ydim, z);    
    edges[k++] = staticpoint(x, y, z+zdim);
    edges[k++] = staticpoint(x+xdim, y, z+zdim);
    edges[k++] = staticpoint(x+xdim, y+ydim, z+zdim);
    edges[k++] = staticpoint(x, y+ydim, z+zdim);
    r = sqrt(_sqr(xdim*0.5)+_sqr(ydim*0.5)+_sqr(zdim*0.5));
    center = staticpoint(x+xdim/2, y+ydim/2, z+zdim/2);
  }
  
  box(point & pt) {
    r = 0;
    center = staticpoint(pt.coord[0], pt.coord[1], pt.coord[2]);
    for(int k=0; k<BOXEDGES; k++) edges[k] = center;
  }
};


class treenode {
public:
  treenode * left;
  treenode * right;
  treenode * parent;
  box boundingbox;
  int depth;
  double lmax;
  double totalmass;
  staticpoint masscenter;
  std::list<treenode *> * interacts;
  int point_idx;
  
  treenode(treenode * p) { parent = p; interacts = NULL;  }
  treenode(treenode *  l, treenode *  r, treenode *  p) { left = l; right = r; parent = p;interacts = NULL;   }
  treenode(treenode *  l, treenode *  r, treenode *  p, int  pt_idx) { left = l; right = r; parent = p; 
    point_idx = pt_idx;  interacts = NULL; }
  
  ~treenode() {
    if (interacts != NULL) delete(interacts);
  }
  
  void add_interaction(treenode * nd) {
     if (interacts == NULL) interacts = new std::list<treenode *>();
     interacts->push_back(nd);
  }
  
 };

struct pointset_part {
  int begin;
  int length;
  pointset_part(int b, int l) { begin = b; length = l; } 
};

class CKPointSet {
  
  
public:
  
  CKPointSet(): s(2.01) {
    pointidx = NULL;
    workarray = NULL;
    internal_nodes = NULL;
    leaves = NULL;
    root = NULL;
  }
  
  ~CKPointSet() {
     free(pointidx);
     free(workarray);
     free(leaves);
     free(internal_nodes);
  }
  
  void reserve(int n) {
    points.resize(n);
  } 
  
  void addPoint(int i, double x, double y, double z) {
    points[i] = point(x,y,z);
  }
   
  
  // Return the min and max value of a point for given dimension
  std::pair<double, double> pointset_dim(int dim) {
    std::vector<point>::iterator iter;
    double dmin = 1e30, dmax=-1e30;
    for(iter=points.begin(); iter!=points.end(); ++iter) {
      dmin = std::min(dmin, iter->coord[dim]);
      dmax = std::max(dmax, iter->coord[dim]);
    }
    return std::pair<double, double>(dmin, dmax);
  }
  
  std::vector<point> & getPoints() {
    return points;
  }
  
  void recompute() {
    boxes.clear();
    buildTree();
    buildWSR();
  }
  
  
  
  std::vector<box> & getBoxes() {
    return boxes;
  }
  
  std::vector< std::pair<staticpoint, staticpoint> > &  getPairs() {
    return pairs;
  }
  
  
  void buildWSR() {
    pairs.clear();
    wsr(root);
   // std::cout << "Pairs size: " << pairs.size() << " nodes: " << treetop << std::endl;
  }
  
  
  int nearest_neighbor_naive(int pntidx) {
    point & p = points[pntidx];
    double minDistSqr = 1e30;
    int minDistIdx = -1;
    for(int i=points.size()-1; i>=0; i--) {
      if (i != pntidx) {
        double r2 = p.rsqr(points[i]);
        if (r2 < minDistSqr) {
          minDistSqr = r2;
          minDistIdx = i;
        }
      }
    }
    return minDistIdx;
  }
  
  void compute_nn_naive() {
    int psz = points.size();
 #ifdef CILK
    cilk_for(int i=0; i<psz; i++) {
 #else
    for(int i=0; i<psz; i++) {
  #endif
      points[i].nn = nearest_neighbor_naive(i); 
    }
  }
  
  std::pair<int,double> mindist(point & p, treenode * node, int minDistIdx, double minDistSqr){ 
    if (node->left == NULL) {
      double r2 = p.rsqr(points[node->point_idx]);
      if (r2<minDistSqr) return std::pair<int,double>(node->point_idx, r2);
      else return std::pair<int,double>(minDistIdx, minDistSqr);
    } else {
      std::pair<int,double> minleft = mindist(p, node->left, minDistIdx, minDistSqr);
      return mindist(p, node->right, minleft.first, minleft.second);
    }
  }
  
  // Check only via interacting edges
  int nearest_neighbor(int pntidx) {
    return points[pntidx].nn;
  }
  
  void compute_wsr_nn() {
    int psz = points.size();
   #ifdef CILK
      cilk_for(int pntidx=0; pntidx<psz; pntidx++){ 
   #else 
      for(int pntidx=0; pntidx<psz; pntidx++){
   #endif
        double minDistSqr = 1e30;
        int minDistIdx = -1;
        point & p = points[pntidx];
        std::list<treenode *> & interacts = *(p.node->interacts);
        std::list<treenode *>::iterator iter;
        //std::cout << "Interacts: " << pntidx << " " << interacts.size() << std::endl;
        for(iter = interacts.begin(); iter != interacts.end(); iter++) {
          std::pair<int, double> mind = mindist(p, *iter, minDistIdx, minDistSqr);
          minDistIdx = mind.first;
          minDistSqr = mind.second;
          //std::cout << pntidx << " : " <<  minDistIdx << " : " << minDistSqr << std::endl;
        }
        assert(minDistIdx >= 0);
        p.nn = minDistIdx;
    }
  }
  
  void visit(treenode * nd, long * n, int * maxdepth) {
     if (nd->interacts != NULL) (*n) += nd->interacts->size();
     (*maxdepth) = std::max(*maxdepth, nd->depth);
     if (nd->left != NULL) visit(nd->left, n, maxdepth);
     if (nd->right != NULL) visit(nd->right,n, maxdepth);
  }
  
  std::pair<long,long> max_depth_and_interedges() {
    long nnodes = 0;
    int maxdepth = 0;
    visit(root, &nnodes, &maxdepth);
    return std::pair<long,int>(nnodes,maxdepth);
  }
    
private:
  std::vector<point> points;
  std::vector< std::pair<staticpoint, staticpoint> > pairs;
  
  int * pointidx;
  int * workarray;
  std::vector<box> boxes;
  treenode * internal_nodes;
  treenode * leaves;
  treenode * root;
  double s;
  
  // Building the tree (using the simpler method, so it is easier to parallize)
  void buildTree() {
    int tsz = 0;
    pointidx = (int*) realloc(pointidx, points.size()*sizeof(int));
    workarray = (int *) realloc(workarray, points.size()*sizeof(int));
    leaves = (treenode *) realloc(leaves, points.size()*sizeof(treenode));
    internal_nodes = (treenode *) realloc(internal_nodes, points.size()*sizeof(treenode));
    for(int i=0; i<points.size(); i++) {
        pointidx[i] = i;
    }
    root = btree(pointset_part(0, points.size()), NULL, 0);
  }
  
  // Nodes are preallocated to avoid doing mallocs (as new()).
  // Leaves have their own array, leaves are indexed by their point index.
  treenode *  leaf(treenode *  parent, int ptidx) {
    treenode * node = &leaves[ptidx];
    *node = treenode(NULL, NULL, parent, ptidx);
    points[ptidx].node = node;
    node->boundingbox = box(points[ptidx]);
    node->lmax = 0;
    if (parent == NULL) {
       node->depth = 0;
    } else {
       node->depth = parent->depth+1;
    }
    return node;
  }
  
  // Internal nodes are indexed by the pivot position when a pointset
  // is split.
  treenode * newnode(treenode * parent, int splitpos) {
    treenode * node = &internal_nodes[splitpos];
    *node = treenode(parent);
        
     if (parent == NULL) {
       node->depth = 0;
     } else {
       node->depth = parent->depth+1;
     }
      
    return node;
  }
  
  std::pair<double, double> pointpart_dim(pointset_part part, int dim) {
    double dmin = 1e30, dmax=-1e30;
    for(int i=0; i<part.length; i++) {
      point & pt = points[pointidx[i+part.begin]];
      dmin = std::min(dmin, pt.coord[dim]);
      dmax = std::max(dmax, pt.coord[dim]);
    }
    return std::pair<double, double>(dmin, dmax);
  }
  
  box pointpart_box(pointset_part part) {
    std::pair<double,double> xd = pointpart_dim(part,0);
    std::pair<double,double> yd = pointpart_dim(part,1);
    std::pair<double,double> zd = pointpart_dim(part,2);
    return box(xd.first,xd.second-xd.first, yd.first, yd.second-yd.first, zd.first, zd.second-zd.first); 
  }
  
  double lmax(box bb) {
    double mx = 0;
    for(int i=1; i<BOXEDGES; i++) {
      for(int d=0; d<DIMENSIONS; d++) {
        mx = std::max(mx, fabs(bb.edges[i].coord[d]-bb.edges[i-1].coord[d]));
      }
    }
    return mx;
  }
  
  std::pair<int, double> maxpart_dim(pointset_part part) {
    double maxdim = 0;
    double maxdimmin = 0;
    int maxdimnum = -1;
    for(int i=0; i<DIMENSIONS; i++) {
      std::pair<double,double> dm = pointpart_dim(part, i);
      if (dm.second-dm.first >= maxdim) {
        maxdim = dm.second-dm.first;
        maxdimmin = dm.first;
        maxdimnum = i;
      }
    }
    return std::pair<int,double>(maxdimnum, maxdimmin + maxdim*0.5);
  }
  
  treenode * btree(pointset_part p, treenode * parent, int level) {
    assert(p.length>=1);
    if (p.length == 1) {
      return leaf(parent, pointidx[p.begin]);
    } else {
      std::pair<int,double> dmax = maxpart_dim(p);
      // Split
      double midpoint = dmax.second;
      int end = p.begin + p.length;
      int c=p.begin;
      for(int i=p.begin; i<end; i++) {
        if (points[pointidx[i]].coord[dmax.first] < midpoint) {
          workarray[c++] = pointidx[i];
        }
      }
      int p2start=c;
      for(int i=p.begin; i<end; i++) {
        if (points[pointidx[i]].coord[dmax.first] >= midpoint) {
          workarray[c++] = pointidx[i];
        }
      }
      // Move to correct parts of the array
      for(int i=p.begin; i<end; i++) {
        pointidx[i] = workarray[i];
      }
      treenode * node = newnode(parent, p2start);
      node->boundingbox = pointpart_box(p);
      node->lmax = lmax(node->boundingbox);
  
      #ifdef CILK
        treenode * leftn;
        treenode * rightn;
        if (level < MAXPARALLELDEPTH) {
            leftn = cilk_spawn btree(pointset_part(p.begin, p2start-p.begin), node, level+1);
            rightn =   btree(pointset_part(p2start, c-p2start), node, level+1);
            cilk_sync;
        } else {
            rightn = btree(pointset_part(p2start, c-p2start), node, level+1);
            leftn = btree(pointset_part(p.begin, p2start-p.begin), node, level+1);
        }
        node->left = leftn;
        node->right = rightn;
      #else
        node->left = btree(pointset_part(p.begin, p2start-p.begin), node, level+1);
        node->right = btree(pointset_part(p2start, c-p2start), node, level+1);
      #endif
      // For visualization:
      //boxes.push_back(tree[newnodeid].boundingbox);
      return node;
    }
  }
  
  bool wellSeparated(box & b1, box & b2) {
    // Smallest radius of a ball that can contain aither rectangle
    double r = std::max(b1.r, b2.r);
    double d = sqrt(b1.center.rsqr(b2.center))-2*r;
    return (d>s*r);
  }
  
  bool wellSeparated(point & p, box & b2, double s) {
    double r = b2.r;
    double d = sqrt(b2.center.rsqr(p))-2*r;
    return (d>s*r);
    
  }
  
  
  void wsr(treenode * node) {
    if (node->left == NULL) {
      // is leaf
      return;
    } else {
      #ifdef CILK
        cilk_spawn wsr(node->left);
        wsr(node->right);
        cilk_sync;
      #else
        wsr(node->left);
        wsr(node->right);
      #endif
      wsrPair(node->left, node->right);
    }
  }
  
  void wsrPair(treenode * t1, treenode * t2) {
    if (wellSeparated(t1->boundingbox, t2->boundingbox)) {
      if (t1->left == NULL) t1->add_interaction(t2);
      if (t2->left == NULL) t2->add_interaction(t1);
//      pairs.push_back(std::pair<staticpoint,staticpoint> (t1.boundingbox.center, t2.boundingbox.center));
    } else if ((t1->lmax) > (t2->lmax)) {
      wsrPair(t1->left, t2);
      wsrPair(t1->right, t2);
    } else {
      wsrPair(t1, t2->left);
      wsrPair(t1, t2->right);
    }
  }
  
};

#endif
