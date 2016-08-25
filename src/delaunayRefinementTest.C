#include <iostream>
#include <fstream>
#include "cilk.h"
#include "delaunay.h"
#include "geomGen.h"
#include "gettime.h"
using namespace std;

tri* delaunay(vertex** v, int n, int);
void checkDelaunay(tri*, vertex**, int);

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;
  
  if (argc > 3 && argv[1][0] == 'f'){
    n = atoi(argv[3]);
    char* filename = (char*)argv[2];
    ifstream file (filename,ios::in);
    utils::myAssert(file.is_open(),"Unable to open file");
    vertex** v = new vertex*[n];
    double x,y;double z;
    int k = 0;
    while(k < n) {
      file >> x; file >> y;
      if(file.eof()) {
	cout<<"File size is only "<<k<<endl;
	file.close();
	abort();
      }
      v[k++] = new vertex(point2d(x,y),k);
    }
    file.close();
    
    startTime();
    tri* T = delaunay(v,k,k);
    stopTime(.1,"Delaunay (from file)");
    free(T);
    for(int i=0;i<k;i++)free(v[i]);
    free(v);
  }
  else {
    /*
    {
      vertex** v = new vertex*[n];
      for (int i=0; i < n; i++) 
	v[i] = new vertex(dataGen::rand2d(i),i);  
      //for(int i=0;i<n;i++)v[i]->print();
      startTime();
      tri* T = delaunay(v,n,n);
      stopTime(.1,"Delaunay (random points in square)");
      //checkDelaunay(T,v,n);
      free(T);  // something causes segfault when freed??
      for(int i=0;i<n;i++) free(v[i]);
      free(v);
    }
    */
    {
      vertex** v = new vertex*[n];
      for (int i=0; i < n; i++) 
	v[i] = new vertex(dataGen::randKuzmin(i),i);    
      startTime();
      int numRefinePoints = 3*n;
      tri* T = delaunay(v,n,numRefinePoints);
      stopTime(.1,"Delaunay (random points in kuzmin distribution)");
      //checkDelaunay(T,v,n);
      free(T);
      for(int i=0;i<n;i++) free(v[i]);
      free(v);
    }
    reportTime("Delaunay (weighted average)");
  }
}
