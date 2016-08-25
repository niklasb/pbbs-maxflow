#include "nearestNeighbor.h"
#include "PointGenerator.h"
#include "utils.h"
#include "../../../src/geom.h"

int _npoints;

template <class PT, int KK>
struct vertex {
  typedef PT pointT;
  pointT pt;         // the point itself
  vertex* ngh[KK];    // the list of neighbors
  vertex(pointT p) {pt=p;}
};

typedef vertex<point3d,1> vertex3d1;

vertex3d1** v;
vertex3d1* vv;

void prepareForPoints(size_t numOfPoints) {
    v = newA(vertex3d1*, numOfPoints);
    vv = newA(vertex3d1, numOfPoints);
    _npoints = numOfPoints;   
}

void addPoint(int i, double3 p) {
    v[i] = new (&vv[i]) vertex3d1(point3d(p[0], p[1],p[2]));
}

void computeNN(int k) {
    assert(k == 1);
    // Does the work
    std::cout << "Start computation " << std::endl;
    ANN<1>(v, _npoints, k);
}

// Used for correctness check.
double3 getNearestPoint(int pointidx) {
    vertex3d1 nv = *(vv[pointidx].ngh[0]);
    return double3(nv.pt.x, nv.pt.y, nv.pt.z);
}
 
 

