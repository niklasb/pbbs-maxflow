#include <iostream>
#include "sequence.h"
#include "cilk.h"
#include "nbody.h"
#include "quadOctTree.h"
using namespace std;

#define ALPHA 1

// *************************************************************
//    STATISTICS
// *************************************************************

int gDirectInteractions = 0; // counts number of direct interactions
int gIndirectInteractions = 0; // counts number of indirect interactions
int sequential = 0;

// *************************************************************
//    FORCE CALCULATIONS
// *************************************************************

struct centerMass {
  double mass;
  centerMass(double m) : mass(m) {}
  centerMass() : mass(0.0) {}
  centerMass operator+(centerMass op2) {
    return centerMass(mass+op2.mass);}
  centerMass operator+(particle* op2) {
    return centerMass(mass+op2->mass);}
};

// Create an octree type with 
//   point type = point3d
//   vector type = vect3d
//   type of elements stored in the tree = particle
//   type of summary data at each internal node = centerMass
typedef gTreeNode<point3d,vect3d,particle,centerMass> octTree;

// Calculate the force from p to nodes in an octree T
vect3d forceTo(particle *p, octTree *T, double alpha) {
  vect3d v = T->center - p->pt;
  double r = v.Length();

  // If far enough away, approximate force on certer of mass of T
  if (r > alpha * T->size) {
    if (sequential) gIndirectInteractions++;
    return v * (p->mass * T->data.mass * gGrav / (r*r*r));
  }

  else {
    vect3d force(0.0, 0.0, 0.0);

    // If node is a leaf loop over the particles in the leaf
    if (T->IsLeaf()) {
      for (int i = 0; i < T->count; i++) {
	particle *pb = T->vertices[i];
	if (pb != p) {
	  vect3d v = (pb->pt) - (p->pt);
	  double r = v.Length();
	  force = force + (v * (p->mass * pb->mass * gGrav /(r*r*r)));
	} else if (sequential) gDirectInteractions--;
      }
      if (sequential) gDirectInteractions += T->count; 
      return force;
    }

    // Otherwise loop over children of the tree
    else {
      for (int i=0; i < 8 ; i++)
	force = force + forceTo(p,T->children[i],alpha);
      return force;
    }
  }
}

// *************************************************************
//   STEP
// *************************************************************

// takes one step and places forces in particles[i]->force
void stepBH(seq<particle*> particles, double alpha) {
 
  // generate the oct tree
  octTree *myTree = octTree::gTree(particles);

  // this reorders the particles for locality
  seq<particle*> p2 = myTree->flatten();

  // generate all the forces
  cilk_for(int i=0; i < particles.size(); i++)
    p2[i]->force = forceTo(p2[i],myTree,alpha);
  myTree->del();
}

void step(seq<particle*> particles) { stepBH(particles, ALPHA); }
