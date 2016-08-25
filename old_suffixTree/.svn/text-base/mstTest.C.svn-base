#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "gettime.h"
#include "graphGen.h"
#include "cilk.h"
using namespace std;

#define DETAILED_TIME 0

//void reportDetailedTime();
wghEdgeArray mst(wghEdgeArray G);

int cilk_main(int argc, char* argv[]) {
  int n, m, dimension;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;
  if (argc > 2) m = std::atoi(argv[2]); else m = 6;

  {
    wghEdgeArray E = wghEdge2dMesh(n);
    startTime();
    wghEdgeArray T = mst(E);
    stopTime(.1,"Minimum Spanning Tree (2d grid, m = 2n)");
    E.del(); T.del();
  }

  {
    edgeArray E1 = edgeRmat(n,5*n,0,.5,.1,.1,.3);
    wghEdgeArray E = addRandWeights(E1);
    E1.del();
    startTime();
    wghEdgeArray T = mst(E);
    stopTime(.1,"Minimum Spanning Tree (rMat m = 5n)");
    E.del(); T.del();
  }

  {
    wghEdgeArray E = wghEdgeRandom(n,n*m);
    startTime();
    wghEdgeArray T = mst(E);
    stopTime(.1,"Minimum Spanning Tree (random, m = 6n)");
    E.del(); T.del();
  }

  reportTime("Minimum Spanning Tree (weighted average)");
}
