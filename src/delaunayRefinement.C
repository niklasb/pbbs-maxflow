#include <iostream>
#include <vector>
#include "seq.h"
#include "nearestNeighbor.h"
#include "cilk.h"
#include "delaunay.h"
#include "gettime.h"
using namespace std;

timer triangulationTimer;

double minAngle(point2d a, point2d b, point2d c) {
  double sideAB = sqrt((double) (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
  double sideBC = sqrt((double) (b.x-c.x)*(b.x-c.x)+(b.y-c.y)*(b.y-c.y));
  double sideCA = sqrt((double) (c.x-a.x)*(c.x-a.x)+(c.y-a.y)*(c.y-a.y));
  double r = 57.2957795;
  double a1 =r*acos((sideAB*sideAB+sideCA*sideCA-sideBC*sideBC)/(2*sideAB*sideCA));
  double a2 = r*acos((sideAB*sideAB+sideBC*sideBC-sideCA*sideCA)/(2*sideAB*sideBC));  
  double a3 = (float) r*acos((sideBC*sideBC+sideCA*sideCA-sideAB*sideAB)/(2*sideBC*sideCA));  
  return min(a1,min(a2,a3));
}

bool inTriangleTest(vertex *p, simplex t){
  for (int i=0; i < 3; i++) {
    t = t.rotClockwise();
    if (t.outside(p)) return false;
  }
  return true;
}

point2d triangleCircumcenter(point2d a, point2d b, point2d c){
 float x = (a.y*a.y+a.x*a.x)*(b.y-c.y)+(b.y*b.y+b.x*b.x)*(c.y-a.y)
    + (c.y*c.y+c.x*c.x)*(a.y-b.y);
  float y = (a.y*a.y+a.x*a.x)*(c.x-b.x)+(b.y*b.y+b.x*b.x)*(a.x-c.x)
    + (c.y*c.y+c.x*c.x)*(b.x-a.x);
  float d = 2*(a.x*(b.y-c.y)+b.x*(c.y-a.y)+c.x*(a.y-b.y));
  x = x/d;
  y = y/d;
  return point2d(x,y);
}

// *************************************************************
//    INCREMENTAL INSERT
// *************************************************************

simplex find(vertex *p, simplex start) {
  simplex t = start;
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
  vector<vertex*> vertexQ;
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
    q->vertexQ.push_back(t.firstVertex());
    t = t.rotClockwise();
    findCavity(t.across(), p, q);
  }
}

// Finds the cavity for v and tries to reserve vertices on the 
// boundary (v must be inside of the simplex t)
// The boundary vertices are pushed onto q->vertexQ and
// simplices to be deleted on q->simplexQ (both initially empty)
// It makes no side effects to the mesh other than to X->reserve
void insertReserve(vertex *v, simplex t, Qs *q) {
  // each iteration searches out from one edge of the triangle
  for (int i=0; i < 3; i++) {
    q->vertexQ.push_back(t.firstVertex());
    findCavity(t.across(), v, q);
    t = t.rotClockwise();
  }
  // the maximum id new vertex that tries to reserve a boundary vertex 
  // will have its id written.  reserve starts out as -1
  for (int i = 0; i < q->vertexQ.size(); i++)
    utils::writeMax(&((q->vertexQ)[i]->reserve), v->id);
}

// checks if v "won" on all adjacent vertices and inserts point if so
bool insert(vertex *v, simplex t, Qs *q) {
  bool flag = 0;
  for (int i = 0; i < q->vertexQ.size(); i++) {
    vertex* u = (q->vertexQ)[i];
    if (u->reserve == v->id) u->reserve = -1; // reset to -1
    else flag = 1; // someone else with higher priority reserved u
  }
  if (!flag) {
    // the following 3 lines do all the side effects to the mesh.
    t.split(v);
    for (int i = 0; i<q->simplexQ.size(); i++) {
      (q->simplexQ)[i].flip();
    }
  }
  q->simplexQ.clear();
  q->vertexQ.clear();
  return flag;
}

bool skinnyTriangle(tri *t) {
  float minAngleThreshold = 20.0;
  return (minAngle(t->vtx[0]->pt, t->vtx[1]->pt, t->vtx[2]->pt) < minAngleThreshold);
}

// returns the indices of the triangles that are bad  
seq<int> collectBadTriangles(tri* Triangs, int n, simplex boundingTriangle) {
  bool *aflags = newA(bool, n);
  cilk_for (int i=0; i<n; i++) {
    tri *t = &Triangs[i];

    // only needs to check triangles that are modified
    if (t->modified && skinnyTriangle(t)) {
      point2d center = triangleCircumcenter(t->vtx[0]->pt, t->vtx[1]->pt, t->vtx[2]->pt);

      //if circumcenter is outside mesh boundary, then skip it
      vertex v(center,0);
      if (inTriangleTest(&v, boundingTriangle)) aflags[i] = 1;
      else aflags[i] = 0;
    } else aflags[i] = 0;

    // reset to unmodified 
    t->modified = 0;
  }

  int* badT = newA(int, n);
  int b = sequence::packIndex(badT, aflags, n);
  free(aflags);
  return seq<int>(badT,b);
}

//allocate the refinement points and store one of the triangle
//vertices as the "closest" point to avoid doing a point location
//during the triangulation
void allocateSteinerPoints(int* badT, int n, vertex** v, tri* Triangs) {
  cilk_for (int i=0; i<n; i++) {
    int j = badT[i];
    tri *t = &Triangs[j]; 
    v[i]->pt = triangleCircumcenter(t->vtx[0]->pt, t->vtx[1]->pt, t->vtx[2]->pt);
    v[i]->badTriangle = t;
  }
}

//associate 2 triangles to each vertex, for the two triangles
//that the vertex will create when added to existing
//triangulation
inline void brokenCilk1(tri* Triangs, vertex** v, int nVertices, int nTriangles){
  cilk_for (int i=0; i < nTriangles; i++)
    Triangs[i].modified = Triangs[i].initialized = 0;

  cilk_for(int i=0; i < nVertices; i++) 
    v[i]->t = Triangs + 2*i; 
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

//closest is NULL for initial triangulation,
//and closest[i] stores the "closest" point for v[i]
//during refinement rounds
int triangulate(int n, vertex** v, kNearestNeighbor<vertex,1> &knn) {
  int top = n;
  int maxR = (int) (n/100) + 1; // maximum number to try in parallel
  Qs *qqs = newA(Qs,maxR);
  Qs **qs = newA(Qs*,maxR);
  for (int i=0; i < maxR; i++) {
    qs[i] = new (&qqs[i]) Qs;
  }
  simplex *t = newA(simplex,maxR);
  bool *flags = newA(bool,maxR);
  vertex** h = newA(vertex*,maxR);
 
  int multiplier = 8; // when to regenerate
  int nextNN = multiplier;

  int rounds = 0; int failed = 0;

  // process all vertices starting just below top
  while(top > 0) {

    // every once in a while create a new point location
    // structure using all points inserted so far
    if ((n-top)>=nextNN && (n-top) < n/multiplier) {
      knn.del();
      knn = kNearestNeighbor<vertex,1>(seq<vertex*>(v+top,n-top));
      nextNN = nextNN*multiplier;
    }

    // determine how many vertices to try in parallel
    int cnt = 1 + (n-top)/100;  // 100 is pulled out of a hat
    cnt = (cnt > maxR) ? maxR : cnt;
    cnt = (cnt > top) ? top : cnt;
    vertex **vv = v+top-cnt;

    // for trial vertices find containing triangle, determine cavity 
    // and reserve vertices on boundary of cavity
    cilk_for (int j = 0; j < cnt; j++) {
      vertex *u = knn.nearest(vv[j]);
      t[j] = find(vv[j],simplex(u->t,0));
      insertReserve(vv[j],t[j],qs[j]);
    }

    // For trial vertices check if they own their boundary and
    // update mesh if so.  flags[i] is 1 if failed (need to retry)
    cilk_for (int j = 0; j < cnt; j++) {
      flags[j] = insert(vv[j],t[j],qs[j]);
    }

    // Pack failed vertices back onto Q and successful
    // ones up above (needed for point location structure)
    int k = sequence::pack(vv,h,flags,cnt);
    cilk_for (int j = 0; j < cnt; j++) flags[j] = !flags[j];
    sequence::pack(vv,h+k,flags,cnt);
    for (int j = 0; j < cnt; j++) vv[j] = h[j];

    failed += k;
    top = top-cnt+k; // adjust top, accounting for failed vertices
    rounds++;
  }  
  free(qqs); free(qs); free(t);
  free(flags); free(h); 
}

int refine(vertex** v, int n, int nTotal) {
  int top = n;
  int maxR = (int) (nTotal/400) + 1; // maximum number to try in parallel
  Qs *qqs = newA(Qs,maxR);
  Qs **qs = newA(Qs*,maxR);
  for (int i=0; i < maxR; i++) {
    qs[i] = new (&qqs[i]) Qs;
  }
  simplex *t = newA(simplex,maxR);
  bool *flags = newA(bool,maxR);
  vertex** h = newA(vertex*,maxR);
 
  int rounds = 0; int failed = 0;

  // process all vertices starting just below top
  while(top > 0) {
    int cnt = min(maxR,top);
    vertex **vv = v+top-cnt;

    // for trial vertices find containing triangle, determine cavity 
    // and reserve vertices on boundary of cavity
    cilk_for (int j = 0; j < cnt; j++) {
      if (!vv[j]->badTriangle->modified) {
	vertex *u = vv[j]->badTriangle->vtx[0];
	t[j] = find(vv[j],simplex(u->t,0));
	insertReserve(vv[j],t[j],qs[j]);
	flags[j] = 1;
      } else
	flags[j] = 0;
    }

    // For trial vertices check if they own their boundary and
    // update mesh if so.  flags[i] is 1 if failed (need to retry)
    cilk_for (int j = 0; j < cnt; j++) {
      if (flags[j] == 1) 
	flags[j] = insert(vv[j],t[j],qs[j]);
    }

    // Pack failed vertices back onto Q
    int k = sequence::pack(vv,h,flags,cnt);
    for (int j = 0; j < k; j++) vv[j] = h[j];
    failed += k;
    top = top-cnt+k; // adjust top, accounting for failed vertices
    rounds++;
  }
  free(qqs); free(qs); free(t);
  free(flags); free(h); 
}

tri *delaunay(vertex** v, int n, int numRefinePoints) {
  startTime();  
  point2d minP = sequence::mapReduce<point2d>(v,n,minpt(),getPt());
  point2d maxP = sequence::mapReduce<point2d>(v,n,maxpt(),getPt());
  double size = (maxP-minP).Length();
  double scale = 10.*size;

  point2d corner0 = point2d(minP.x-scale, minP.y-scale);
  point2d corner1 = point2d(minP.x + scale, minP.y);
  point2d corner2 = point2d(minP.x, minP.y + scale);

  // The following three vertices form a bounding triangle
  vertex *v1 = new vertex(corner0, -3);
  vertex *v2 = new vertex(corner1, -2);
  vertex *v3 = new vertex(corner2, -1);

  int totalVertices = n + numRefinePoints;
  int totalTriangles = 2*totalVertices +1;

  vertex** vv = newA(vertex*, totalVertices);
  vertex* vI = newA(vertex, numRefinePoints);
  cilk_for(int i=0;i<n;i++) vv[i] = v[i];
  cilk_for (int i=n; i < n+numRefinePoints; i++){ 
    vv[i] = new (&vI[i-n]) vertex(point2d(0,0),i);
  }

  tri* Triangs = newA(tri, totalTriangles);
  simplex s = simplex(v1,v2,v3,Triangs); // use first one for initial triang

  brokenCilk1(Triangs + 1, vv, totalVertices, totalTriangles);

  // create a point location structure
  //typedef kNearestNeighbor<vertex,1> KNN;
  kNearestNeighbor<vertex,1> knn = 
    kNearestNeighbor<vertex,1>(seq<vertex*>(&v1,1));
  nextTime("initialize");

  triangulate(n, vv, knn);
  nextTime("delaunay");
  knn.del();
  
  int numTriangs = 2*n+1;
  int rround = 0;
  int top = n;

  //temporary objects storing the initial boundary
  //used to check if new points lie outside the boundary
  tri* tempt = new tri;
  simplex boundingSimplex = simplex(v1,v2,v3,tempt);

  while (1) {
    seq<int> badT = collectBadTriangles(Triangs, numTriangs, boundingSimplex);
    //nextTime("bad");

    int numBad = badT.sz;
    cout << "numBad = " << numBad << endl;
    if (numBad == 0) break;

    allocateSteinerPoints(badT.S, numBad, vv+top, Triangs);
    badT.del();
    //nextTime("bad");

    refine(vv+top, numBad, top);
    //nextTime("refine");
    top += numBad;
    numTriangs += 2*numBad;
  }
  nextTime("refinement");

  int* counts = newA(int, numTriangs);
  int* bad = newA(int, numTriangs);
  cilk_for (int i = 0; i < numTriangs; i++) {
    tri* t = &Triangs[i];
    counts[i] = t->initialized;
    if (t->initialized && skinnyTriangle(t)) {
      point2d center = triangleCircumcenter(t->vtx[0]->pt, t->vtx[1]->pt, t->vtx[2]->pt);
      vertex v(center,0);
      bad[i] = inTriangleTest(&v, boundingSimplex);
    } else bad[i] = 0;
  }
  int m = sequence::plusReduce(counts, numTriangs);
  int nbad = sequence::plusReduce(bad, numTriangs);

  if (nbad > 0)
    cout << "error: " << nbad << " bad triangles remain" << endl;

  cout << "original triangles = " << 2*n+1 << endl;
  cout << "refinement triangles = " << m - (2*n+1) << endl;
}
