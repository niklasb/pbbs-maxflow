#include <iostream>
#include <limits>
#include "seq.h"
#include "cilk.h"
//#include "quadOctTree.h"
//#include "pOctTree.h"
#include "ppOctTree.h"
#include "quickSort.h"
#include "geomGen.h"
using namespace std;

// *************************************************************
//  SOME DEFINITIONS
// *************************************************************

class particle3d {
public:
  point3d pt;
  int id;
  vect3d force;
  particle3d(point3d p, int i) : pt(p), id(i) {}
};

class particle2d {
public:
  point2d pt;
  int id;
  vect2d force;
  particle2d(point2d p, int i) : pt(p), id(i) {}
};

// Create an octree type with 
//   point type = point3d
//   vector type = vect3d
//   type of elements stored in the tree = particle
typedef gTreeNode<point3d,vect3d,particle3d> octTree;
typedef gTreeNode<point2d,vect2d,particle2d> quadTree;

// *************************************************************
//  MAIN TEST
// *************************************************************

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10000;

  // Warm it up (i.e. make sure allocation pool is available)
  for (int i=0; i < 2; i++) {
    particle2d** p = newA(particle2d*,n);
    particle2d* pp = newA(particle2d,n);
    {cilk_for (int i=0; i < n; i++) 
	p[i] = new (&pp[i]) particle2d(dataGen::rand2d(i),i);}
    quadTree *myTree = quadTree::gTree(seq<particle2d*>(p,n));

    // check correctness on first run
    if (i==0) {
      seq<particle2d*> p2 = myTree->flatten();
      if (p2.size() != n || myTree->data.cnt != n) {
	cout << "buildTree: wrong size" << endl;
	abort();
      }
      bool* F = newA(bool,n);
      cilk_for(int i=0; i < n; i++) F[i] = false;
      for (int j=0; j < n; j++) {
	int id = p2.S[j]->id;
	if (id < 0 || id > n) {
	  cout << "error at i=" << j << endl;
	  abort();
	}
	F[id] = true;
      }
      for (int j=0; j < n; j++) 
	if (F[j] != true) {
      	  cout << "missing id at i=" << j << endl;
	  abort();
	}
      free(F); p2.del();
    }
    
    free(p); free(pp); myTree->del();
  }

  {
    particle2d** p = newA(particle2d*,n);
    particle2d* pp = newA(particle2d,n);
    cilk_for (int i=0; i < n; i++) 
      p[i] = new (&pp[i]) particle2d(dataGen::rand2d(i),0.0);
    startTime();
    quadTree *myTree = quadTree::gTree(seq<particle2d*>(p,n));
    stopTime(.1,"build tree (random 2d)");
    free(p); free(pp); myTree->del();
  }

  {
    particle2d** p = newA(particle2d*,n);
    particle2d* pp = newA(particle2d,n);
    cilk_for (int i=0; i < n; i++) 
      p[i] = new (&pp[i]) particle2d(dataGen::randKuzmin(i),0.0);
    startTime();
    quadTree *myTree = quadTree::gTree(seq<particle2d*>(p,n));
    stopTime(.1,"build tree (kuzmin 2d)");
    free(p); free(pp); myTree->del();
  }
  

  // Warm it up (i.e. make sure allocation pool is available)
  for (int i=0; i < 2; i++) {
    particle3d** p = newA(particle3d*,n);
    particle3d* pp = newA(particle3d,n);
    cilk_for (int i=0; i < n; i++) 
      p[i] = new (&pp[i]) particle3d(dataGen::rand3d(i),0.0);
    octTree *myTree = octTree::gTree(seq<particle3d*>(p,n));
    free(p); free(pp); myTree->del();
  }

  {
    particle3d** p = newA(particle3d*,n);
    particle3d* pp = newA(particle3d,n);
    cilk_for (int i=0; i < n; i++) 
      p[i] = new (&pp[i]) particle3d(dataGen::rand3d(i),0.0);
    startTime();
    octTree *myTree = octTree::gTree(seq<particle3d*>(p,n));
    stopTime(.1,"build tree (random 3d)");
    free(p); free(pp); myTree->del();
  }


  {
    particle3d** p = newA(particle3d*,n);
    particle3d* pp = newA(particle3d,n);
    cilk_for (int i=0; i < n; i++) 
      p[i] = new (&pp[i]) particle3d(dataGen::randPlummer(i),0.0);
    startTime();
    octTree *myTree = octTree::gTree(seq<particle3d*>(p,n));
    stopTime(.1,"build tree (plummer 3d)");
    free(p); free(pp); myTree->del();
  }

  reportTime("build tree (weighted average)");
}
