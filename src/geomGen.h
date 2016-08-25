#include <iostream>
#include "cilk.h"
#include <math.h>
#include "itemGen.h"
#include "geom.h"

// *************************************************************
//    POINT GENERATION
// *************************************************************

namespace dataGen {

point2d rand2d(int i) {
  return point2d(2*hash<double>(2*i)-1,
		 2*hash<double>(2*i+1)-1);
}

point3d rand3d(int i) {
  return point3d(2*hash<double>(3*i)-1,
		 2*hash<double>(3*i+1)-1,
		 2*hash<double>(3*i+2)-1);
}

point2d randInUnitCircle2d(int i) {
  int j = 0;
  vect2d v;
  do {
    int o = hash<int>(j++);
    v = vect2d(rand2d(o+i));
  } while (v.Length() > 1.0);
  return point2d(v);
}

point3d randInUnitCircle3d(int i) {
  int j = 0;
  vect3d v;
  do {
    int o = hash<int>(j++);
    v = vect3d(rand3d(o+i));
  } while (v.Length() > 1.0);
  return point3d(v);
}

point2d randOnUnitCircle2d(int i) {
  vect2d v = vect2d(randInUnitCircle2d(i));
  return point2d(v/v.Length());
}

point3d randOnUnitCircle3d(int i) {
  vect3d v = vect3d(randInUnitCircle3d(i));
  return point3d(v/v.Length());
}

point2d randProjectedParabola2d(int i) {
  vect2d v = vect2d(rand2d(i));
  return point2d(v.x,1-v.Length());
}

 const double PI = atan(1.0)*4;

 point2d randKuzminO(int i) {
   double r, theta;
   theta = 2*PI*hash<double>(2*i);
   r = hash<double>(2*i + 1);
   r = sqrt(1.0/((1.0-r)*(1.0-r))-1.0);
   return point2d(r*sin(theta),r*cos(theta));
 }

 point2d randKuzmin(int i) {
   vect2d v = vect2d(randOnUnitCircle2d(i));
   int j = hash<int>(i);
   double s = hash<double>(j);
   double r = sqrt(1.0/((1.0-s)*(1.0-s))-1.0);
   return point2d(v*r);
 }

 point3d randPlummer(int i) {
   vect3d v = vect3d(randOnUnitCircle3d(i));
   int j = hash<int>(i);
   double s = pow(hash<double>(j),2.0/3.0);
   double r = sqrt(s/(1-s));
   return point3d(v*r);
 }

};
