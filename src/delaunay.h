#include <iostream>
#include "geom.h"

using namespace std;
#include <iomanip>

// *************************************************************
//    TOPOLOGY
// *************************************************************

struct vertex;

// an unoriented triangle with its three neighbors and 3 vertices
struct tri {
  tri *ngh [3];
  vertex *vtx [3];
  bool initialized;
  bool modified;
  void setT(tri *t1, tri *t2, tri* t3) {
    ngh[0] = t1; ngh[1] = t2; ngh[2] = t3; }
  void setV(vertex *v1, vertex *v2, vertex *v3) {
    vtx[0] = v1; vtx[1] = v2; vtx[2] = v3; }
  int locate(tri *t) {
    for (int i=0; i < 3; i++)
      if (ngh[i] == t) return i;
    cout << "did not locate: self consistency error" << endl;
    abort(); 
  }
  void update(tri *t, tri *tn) {
    for (int i=0; i < 3; i++)
      if (ngh[i] == t) {ngh[i] = tn; return;}
    cout << "did not update: self consistency error" << endl;
    abort(); 
  }
};

// a vertex pointing to an arbitrary triangle to which it belongs (if any)
struct vertex {
  typedef point2d pointT;
  point2d pt;
  tri *t;
  tri *badTriangle;
  int id;
  int reserve;
  vertex* neighbor;
  void print() {
    pt.print(); 
    if (t != NULL)
      for (int i=0; i < 3; i++) cout << t->vtx[i]->id << ",";
    cout << endl;
  }
  vertex(point2d p, int i) : pt(p), id(i), reserve(-1), neighbor(NULL) {}
};

// a simplex is just an oriented triangle.  An integer (o)
// is used to indicate which of 3 orientations it is in (0,1,2)
struct simplex {
  tri *t;
  int o;
  simplex(tri *tt, int oo) : t(tt), o(oo) {}
  simplex(vertex *v1, vertex *v2, vertex *v3, tri *tt) {
    t = tt;
    t->ngh[0] = t->ngh[1] = t->ngh[2] = NULL;
    t->vtx[0] = v1; v1->t = t;
    t->vtx[1] = v2; v2->t = t;
    t->vtx[2] = v3; v3->t = t;
    o = 0;
  }

  void print() {
    if (t == NULL) cout << "NULL simp" << endl;
    else {
      cout << "vtxs=";
      for (int i=0; i < 3; i++) 
	if (t->vtx[(i+o)%3] != NULL)
	  cout << std::setprecision(4) << t->vtx[(i+o)%3]->id << " (" <<
	    t->vtx[(i+o)%3]->pt.x << "," <<
	    t->vtx[(i+o)%3]->pt.y << ") ";
      cout << endl;
    }
  }

  simplex across() {
    tri *to = t->ngh[o];
    if (to != NULL) return simplex(to,to->locate(t));
    else return simplex(to,0);
  }

  simplex rotClockwise() { return simplex(t,(o+1)%3);}

  bool valid() {return (t != NULL);}
  
  vertex *firstVertex() {return t->vtx[o];}

  bool inCirc(vertex *v) {
    if (t == NULL) return 0;
    return inCircle(t->vtx[0]->pt, t->vtx[1]->pt, 
		    t->vtx[2]->pt, v->pt);
  }

  bool outside(vertex *v) {
    if (t == NULL) return 0;
    return counterClockwise(t->vtx[(o+2)%3]->pt, v->pt, t->vtx[o]->pt);
  }

  // flips two triangles and adjusts neighboring triangles
  void flip() { 
    simplex s = across();

    tri *t1 = t->ngh[(o+1)%3];
    tri *t2 = s.t->ngh[(s.o+1)%3];
    vertex *v1 = t->vtx[(o+1)%3];
    vertex *v2 = s.t->vtx[(s.o+1)%3];

    t->modified = 1;
    s.t->modified = 1;

    t->vtx[o]->t = s.t;
    t->vtx[o] = v2;
    t->ngh[o] = t2;
    if (t2 != NULL) t2->update(s.t,t);
    t->ngh[(o+1)%3] = s.t;

    s.t->vtx[s.o]->t = t;
    s.t->vtx[s.o] = v1;
    s.t->ngh[s.o] = t1;
    if (t1 != NULL) t1->update(t,s.t);
    s.t->ngh[(s.o+1)%3] = t;
  }

  // splits the triangle into three triangles with new vertex v in the middle
  // updates all neighboring simplices
  // REQUIRES THAT : v->t stores a pointer to two unused adjacent triangles 
  //    that can be used
  void split(vertex* v) {
    //tri *ta0 = new tri[2];
    tri *ta0 = v->t; 
    tri *ta1 = ta0+1;

    ta0->modified = ta0->initialized = 1;
    ta1->modified = ta1->initialized = 1;
    t->modified = 1;

    v->t = t;
    tri *t1 = t->ngh[0]; tri *t2 = t->ngh[1]; tri *t3 = t->ngh[2];
    vertex *v1 = t->vtx[0]; vertex *v2 = t->vtx[1]; vertex *v3 = t->vtx[2];
    t->ngh[1] = ta0;        t->ngh[2] = ta1;
    t->vtx[1] = v;
    ta0->setT(t2,ta1,t);  ta0->setV(v2,v,v1);
    ta1->setT(t3,t,ta0);  ta1->setV(v3,v,v2);
    if (t2 != NULL) t2->update(t,ta0);      
    if (t3 != NULL) t3->update(t,ta1);
    v2->t = ta0;
  }
};




