#ifndef _BENCH_GEOM_INCLUDED
#define _BENCH_GEOM_INCLUDED
#include <iostream>
#include <algorithm>
#include <math.h>
using namespace std;

// *************************************************************
//    POINTS AND VECTORS (2d and 3d)
// *************************************************************

class point3d;

class vect3d {
public: 
  double x; double y; double z;
  vect3d(double xx,double yy, double zz) : x(xx),y(yy),z(zz) {}
  vect3d() {x=0;y=0;z=0;}
  vect3d(point3d p);
  vect3d operator+(vect3d op2) {
    return vect3d(x + op2.x, y + op2.y, z + op2.z);}
  point3d operator+(point3d op2);
  vect3d operator*(double s) {return vect3d(x * s, y * s, z * s);}
  vect3d operator/(double s) {return vect3d(x / s, y / s, z / s);}
  double maxDim() {return max(x,max(y,z));}
  void print() {cout << ":(" << x << "," << y << "," << z << "):";}
  double Length(void) { return sqrt(x*x+y*y+z*z);}
};

class point3d {
public: 
  typedef vect3d vectT;
  double x; double y; double z;
  int dimension() {return 3;}
  point3d(double xx,double yy, double zz) : x(xx),y(yy),z(zz) {}
  point3d() {x=0;y=0;z=0;}
  point3d(vect3d v) : x(v.x),y(v.y),z(v.z) {};
  void print() {cout << ":(" << x << "," << y << "," << z << "):";}
  vect3d operator-(point3d op2) {
    return vect3d(x - op2.x, y - op2.y, z - op2.z);}
  point3d operator+(vect3d op2) {
    return point3d(x + op2.x, y + op2.y, z + op2.z);}
  point3d minCoords(point3d b) {
    return point3d(min(x,b.x),min(y,b.y),min(z,b.z)); }
  point3d maxCoords(point3d b) { 
    return point3d(max(x,b.x),max(y,b.y),max(z,b.z)); }
  int quadrant(point3d center) {
    int index = 0;
    if (x > center.x) index += 1;
    if (y > center.y) index += 2;
    if (z > center.z) index += 4;
    return index;
  }
  // returns a point3d offset by offset in one of 8 directions 
  // depending on dir (an integer from [0..7])
  point3d offsetPoint(int dir, double offset) {
    double xx = x + ((dir & 1) ? offset : -offset);
    double yy = y + ((dir & 2) ? offset : -offset);
    double zz = z + ((dir & 4) ? offset : -offset);
    return point3d(xx,yy,zz);
  }
  // checks if pt is outside of a box centered at this point with
  // radius hsize
  bool outOfBox(point3d pt, double hsize) { 
    return ((x - hsize > pt.x) || (x + hsize < pt.x) ||
	    (y - hsize > pt.y) || (y + hsize < pt.y) ||
	    (z - hsize > pt.z) || (z + hsize < pt.z));
  }

};

inline point3d vect3d::operator+(point3d op2) {
  return point3d(x + op2.x, y + op2.y, z + op2.z);}

inline vect3d::vect3d(point3d p) { x = p.x; y = p.y; z = p.z;}

class point2d;

class vect2d {
public: 
  double x; double y;
  vect2d(double xx,double yy) : x(xx),y(yy) {}
  vect2d() {x=0;y=0;}
  vect2d(point2d p);
  vect2d operator+(vect2d op2) {return vect2d(x + op2.x, y + op2.y);}
  point2d operator+(point2d op2);
  vect2d operator*(double s) {return vect2d(x * s, y * s);}
  vect2d operator/(double s) {return vect2d(x / s, y / s);}
  double maxDim() {return max(x,y);}
  void print() {cout << ":(" << x << "," << y << "):";}
  double Length(void) { return sqrt(x*x+y*y);}
};

class point2d {
public: 
  typedef vect2d vectT;
  double x; double y; 
  int dimension() {return 2;}
  point2d(double xx,double yy) : x(xx),y(yy) {}
  point2d() {x=0;y=0;}
  point2d(vect2d v) : x(v.x),y(v.y) {};
  void print() {cout << ":(" << x << "," << y << "):";}
  vect2d operator-(point2d op2) {return vect2d(x - op2.x, y - op2.y);}
  point2d operator+(vect2d op2) {return point2d(x + op2.x, y + op2.y);}
  point2d minCoords(point2d b) { return point2d(min(x,b.x),min(y,b.y)); }
  point2d maxCoords(point2d b) { return point2d(max(x,b.x),max(y,b.y)); }
  int quadrant(point2d center) {
    int index = 0;
    if (x > center.x) index += 1;
    if (y > center.y) index += 2;
    return index;
  }
  // returns a point2d offset by offset in one of 4 directions 
  // depending on dir (an integer from [0..3])
  point2d offsetPoint(int dir, double offset) {
    double xx = x + ((dir & 1) ? offset : -offset);
    double yy = y + ((dir & 2) ? offset : -offset);
    return point2d(xx,yy);
  }
  bool outOfBox(point2d pt, double hsize) { 
    return ((x - hsize > pt.x) || (x + hsize < pt.x) ||
	    (y - hsize > pt.y) || (y + hsize < pt.y));
  }
};

inline point2d vect2d::operator+(point2d op2) {
  return point2d(x + op2.x, y + op2.y);}

inline vect2d::vect2d(point2d p) { x = p.x; y = p.y;}

// *************************************************************
//    GEOMETRY
// *************************************************************

// Returns twice the area of the oriented triangle (a, b, c)
static double triArea(point2d a, point2d b, point2d c) {
  return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}

// Returns TRUE if the point d is inside the circle defined by the
// points a, b, c. 
static bool inCircle(point2d a, point2d b, point2d c, point2d d) {
  return (a.x*a.x + a.y*a.y) * triArea(b, c, d) -
    (b.x*b.x + b.y*b.y) * triArea(a, c, d) +
    (c.x*c.x + c.y*c.y) * triArea(a, b, d) -
    (d.x*d.x + d.y*d.y) * triArea(a, b, c) > 0;
}

// Returns TRUE if the points a, b, c are in a counterclockise order
static int counterClockwise(point2d a, point2d b, point2d c) {
  return (triArea(a, b, c) > 0);
}

#endif // _BENCH_GEOM_INCLUDED
