#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "gettime.h"
#include "graphGen.h"
#include "cilk.h"
#include <algorithm>
using namespace std;


edgeArray st(edgeArray EA);

int cilk_main(int argc, char* argv[]) {
  int n, degree, dimension;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;
  if (argc > 2) degree = std::atoi(argv[2]); else degree = 10;
  if (argc > 3) dimension = atoi(argv[3]); else dimension = 2;

  {
    edgeArray EA = edgeRandomWithDimension(dimension,degree,n);
    random_shuffle(EA.E,EA.E+EA.nonZeros-1);
    startTime();
    st(EA);
    stopTime(.1,"Spanning Tree (random)");
    EA.del(); 
  }
  {
    edgeArray EA = edge2DMesh(n);
    random_shuffle(EA.E,EA.E+EA.nonZeros-1);

    startTime();
    st(EA);
    stopTime(.1,"Spanning Tree (2d grid)");
    EA.del(); 
  }

  {
    edgeArray EA = edgeRmat(n,5*n,0,.5,.1,.1,.3);
    random_shuffle(EA.E,EA.E+EA.nonZeros-1);

    startTime();
    st(EA);
    stopTime(.1,"Spanning Tree (rMat m = 5n)");
    EA.del(); 
  }


  reportTime("Spanning Tree (weighted average)");
}
