#include "nearestNeighbor.h"
#include "geomGen.h"

#define K 10

// *************************************************************
//  SOME DEFINITIONS
// *************************************************************

template <class PT, int KK>
struct vertex {
  typedef PT pointT;
  pointT pt;         // the point itself
  vertex* ngh[KK];    // the list of neighbors
  vertex(pointT p) {pt=p;}
};

typedef vertex<point3d,K> vertex3d;
typedef vertex<point2d,K> vertex2d;
typedef vertex<point3d,1> vertex3d1;
typedef vertex<point2d,1> vertex2d1;

// *************************************************************
//  MAIN TEST
// *************************************************************

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10000;

  // warm up
  for (int i=0; i < 1 ; i++) {
    vertex2d1** v = newA(vertex2d1*,n);
    vertex2d1* vv = newA(vertex2d1,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex2d1(dataGen::rand2d(i));
    ANN<1>(v, n, 1);
    free(v);
    free(vv);
  }

  {
    vertex2d1** v = newA(vertex2d1*,n);
    vertex2d1* vv = newA(vertex2d1,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex2d1(dataGen::rand2d(i));
    startTime();
    ANN<1>(v, n, 1);
    stopTime(.1,"k-nearest neighbors (random 2d, k=1)");
    free(v);
    free(vv);
  }

  for (int i=0; i < 1; i++) {
    vertex3d1** v = newA(vertex3d1*,n);
    vertex3d1* vv = newA(vertex3d1,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex3d1(dataGen::rand3d(i));
    ANN<1>(v, n, 1);
    free(v);
    free(vv);
  }

  {
    vertex3d1** v = newA(vertex3d1*,n);
    vertex3d1* vv = newA(vertex3d1,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex3d1(dataGen::rand3d(i));
    startTime();
    ANN<1>(v, n, 1);
    stopTime(.1,"k-nearest neighbors (random 3d, k=1)");
    free(v);
    free(vv);
  }

  {
    vertex3d1** v = newA(vertex3d1*,n);
    vertex3d1* vv = newA(vertex3d1,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex3d1(dataGen::randPlummer(i));
    startTime();
    ANN<1>(v, n, 1);
    stopTime(.1,"k-nearest neighbors (plummer 3d, k=1)");
    free(v);
    free(vv);
  }

  {
    vertex2d** v = newA(vertex2d*,n);
    vertex2d* vv = newA(vertex2d,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex2d(dataGen::rand2d(i));
    startTime();
    ANN<K>(v, n, K);
    stopTime(.1,"k-nearest neighbors (random 2d, k=10)");
    free(v);
    free(vv);
  }

  {
    vertex2d** v = newA(vertex2d*,n);
    vertex2d* vv = newA(vertex2d,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex2d(dataGen::randKuzmin(i));
    startTime();
    ANN<K>(v, n, K);
    stopTime(.1,"k-nearest neighbors (kuzmin 2d, k=10)");
    free(v);
    free(vv);
  }
  
  {
    vertex3d** v = newA(vertex3d*,n);
    vertex3d* vv = newA(vertex3d,n);
    cilk_for (int i=0; i < n; i++) 
      v[i] = new (&vv[i]) vertex3d(dataGen::rand3d(i));
    startTime();
    ANN<K>(v, n, K);
    stopTime(.1,"k-nearest neighbors (random 3d, k=10)");
    free(v);
    free(vv);
  }

  reportTime("k-nearest neighbors (weighted average)");
}
