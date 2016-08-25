#include <iostream>
#include "cilk.h"
#include "delaunay.h"
#include "geomGen.h"
#include "gettime.h"
using namespace std;

tri* delaunay(vertex** v, int n);

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;

  {
    vertex** v = new vertex*[n];
    for (int i=0; i < n; i++) 
      v[i] = new vertex(dataGen::rand2d(i),i);    
    startTime();
    tri* T = delaunay(v,n);
    stopTime(.1,"Delaunay (random points in square)");
    free(T);  // something causes segfault when freed??
  }

  {
    vertex** v = new vertex*[n];
    for (int i=0; i < n; i++) 
      v[i] = new vertex(dataGen::randKuzmin(i),i);    
    startTime();
    tri* T = delaunay(v,n);
    stopTime(.1,"Delaunay (random points in kuzmin distribution)");
    free(T);
  }
  reportTime("Delaunay (weighted average)");
}
