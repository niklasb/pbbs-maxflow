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
  while (1) {
    int i;
    for (i=0; i < 3; i++) {//cout<<"finding\n";
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


bool insertReserve(vertex *v, simplex t, Qs *q) {
  // each iteration searches out from one edge of the triangle
  for (int i=0; i < 3; i++) {
    q->vertexQ.push_back(t.firstVertex());
    findCavity(t.across(), v, q);
    t = t.rotClockwise();
  }
  // the maximum id new vertex that tries to reserve a boundary vertex 
  // will have its id written.  reserve starts out as -1
  for (int i = 0; i < q->vertexQ.size(); i++){
   utils::CAS(&((q->vertexQ)[i]->reserve), -1, v->id);
  }
}
// checks if v "won" on all adjacent vertices and inserts point if so
bool insert(vertex *v, simplex t, Qs *q) {
  // each iteration searches out from one edge of the triangle
  
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
  //simplex s = simplex(v1,v2,v3);

  tri* Triangs = newA(tri,2*n+1); // all the triangles needed
  simplex s = simplex(v1,v2,v3,Triangs+2*n); // use last one for initial triang
  for(int i=0; i < n; i++) 
    v[i]->t = Triangs + 2*i; //cilk_for fails??

  // various structures needed for each parallel insertion
  int maxR = (int) (n/100) + 1; // maximum number to try in parallel
  Qs *qqs = newA(Qs,maxR);
  Qs **qs = newA(Qs*,maxR);
  for (int i=0; i < maxR; i++) {
    qs[i] = new (&qqs[i]) Qs;
  }
  simplex *t = newA(simplex,maxR);
  bool *flags = newA(bool,maxR);
  vertex** h = newA(vertex*,maxR);

  // create a point location structure
  typedef kNearestNeighbor<vertex,1> KNN;
  KNN knn = KNN(seq<vertex*>(&v1,1));
  int multiplier = 8; // when to regenerate
  int nextNN = multiplier;

  int top=n; int rounds = 0; int failed = 0;

  // process all vertices starting just below top
  while(top > 0) {
    // every once in a while create a new point location
    // structure using all points inserted so far

    if ((n-top)>=nextNN && (n-top) < n/multiplier) {
      knn.del();
      knn = KNN(seq<vertex*>(v+top,n-top));
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

    //cout<<rounds<<" "<<cnt<< " " <<k<<endl;
    failed += k;
    top = top-cnt+k; // adjust top, accounting for failed vertices
    rounds++;
  }
  knn.del();
  free(qqs); free(qs); free(t); free(flags); free(h);
  cout << "n=" << n << "  Total retries=" << failed
       << "  Total rounds=" << rounds << endl;
  return Triangs;
}

void checkDelaunay(tri* Triangs, vertex** v, int n){
  cilk_for(int i=0;i<2*n+1;i++){
    vertex* v1 = Triangs->vtx[0];
    vertex* v2 = Triangs->vtx[1];
    vertex* v3 = Triangs->vtx[2];
    cilk_for(int j=0;j<n;j++){
      vertex* v4 = v[j];
      if(inCircle(v1->pt,v2->pt,v3->pt,v4->pt)) {
	cout << "in circle" << endl;
	abort();
      }
    }
  }
}




