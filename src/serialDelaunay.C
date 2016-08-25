#include <iostream>
#include <vector>
#include "seq.h"
#include "nearestNeighbor.h"
#include "cilk.h"
#include "delaunay.h"
using namespace std;

// *************************************************************
//    INCREMENTAL INSERT
// *************************************************************

simplex find(vertex *p, simplex start) {
  simplex t = start; 
  int tmp = 0;
  while (1) { 
    int i;
    for (i=0; i < 3; i++) { 
      t = t.rotClockwise();
      if (t.outside(p)) {t = t.across(); break;}
    }
    if (i==3) return t;
  }
}

// holds vertex and simplex queues
struct Qs {
  vector<simplex> simplexQ;
};

// Recursive routine for finding a cavity across an edge with
// respect to a vertex p.
// The simplex has orientation facing the direction it is entered.
//
//         a
//         | \ --> recursive call
//   p --> |T c 
// enter   | / --> recursive call
//         b
//
//  If p is in T then add T to simplexQ, c to vertexQ, and recurse
void findCavity(simplex t, vertex *p, Qs *q) {
  if (t.inCirc(p)) {
    q->simplexQ.push_back(t);
    t = t.rotClockwise();
    findCavity(t.across(), p, q);
    t = t.rotClockwise();
    findCavity(t.across(), p, q);
  }
}


// checks if v "won" on all adjacent vertices and inserts point if so
bool insert(vertex *v, simplex t, Qs *q) {
  for (int i=0; i < 3; i++) {
    findCavity(t.across(), v, q);
    t = t.rotClockwise();
  }

  // the following 3 lines do all the side effects to the mesh.
  t.split(v);
  for (int i = 0; i<q->simplexQ.size(); i++) {
    (q->simplexQ)[i].flip();
  }

  q->simplexQ.clear();
  return 0;
}

  
// *************************************************************
//    MAIN LOOP
// *************************************************************

struct minpt {
  point2d operator() (point2d a, point2d b) {return a.minCoords(b);}};
struct maxpt {
  point2d operator() (point2d a, point2d b) {return a.maxCoords(b);}};
struct getPt {
  point2d operator() (vertex *v) {return v->pt;}};

tri *delaunay(vertex** v, int n) {
  point2d minP = sequence::mapReduce<point2d>(v,n,minpt(),getPt());
  point2d maxP = sequence::mapReduce<point2d>(v,n,maxpt(),getPt());
  point2d corner1 = point2d(minP.x + 2*(maxP.x - minP.x),minP.y);
  point2d corner2 = point2d(minP.x, minP.y + 2*(maxP.y - minP.y));

  // The following three vertices form a bounding triangle
  vertex *v1 = new vertex(minP, -3);
  vertex *v2 = new vertex(corner1, -2);
  vertex *v3 = new vertex(corner2, -1);  

  tri* Triangs = newA(tri,2*n+1); // all the triangles needed
  simplex s = simplex(v1,v2,v3,Triangs+2*n); // use last one for initial triang
  for(int i=0; i < n; i++) 
    v[i]->t = Triangs + 2*i;

  Qs *qqs = newA(Qs,n);
  Qs **qs = newA(Qs*,n);
  for (int i=0; i < n; i++) {
    qs[i] = new (&qqs[i]) Qs;
  }
  simplex *t = newA(simplex,n);

  // create a point location structure
  typedef kNearestNeighbor<vertex,1> KNN;
  KNN knn = KNN(seq<vertex*>(&v1,1));
  int multiplier = 8; // when to regenerate
  int nextNN = multiplier;

  for(int j=n-1;j>=0;j--){
    vertex *u = knn.nearest(v[j]);
    t[j] = find(v[j],simplex(u->t,0));
    insert(v[j],t[j],qs[j]);
    if((n-j)>=nextNN && (n-j) < n/multiplier){
      knn.del();
      knn = KNN(seq<vertex*>(v+j,(n-j)));
      nextNN = nextNN*multiplier;
    }
  }
 
  knn.del();
  free(qqs); free(qs); free(t);

  return Triangs;
}
