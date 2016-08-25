#include <iostream>
#include "sequence.h"
#include "graphGen.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

int* colorGraph(graph G, int seed);

// **************************************************************
//    MAIN
// **************************************************************

int cilk_main(int argc, char *argv[]) {
  int n, m, dimension;
  if (argc > 1) n = atoi(argv[1]); else n = 10;
  if (argc > 2) m = atoi(argv[2]); else m = 10;
  if (argc > 3) dimension = atoi(argv[3]); else dimension = 2;

  {
    graph G = graphRandomWithDimension(dimension, m, n);
    startTime();
    int *Colors = colorGraph(G, 0);
    stopTime(.1,"Color Graph (rand dim=2, m=n*10)");
    G.del(); free(Colors);
  }

  {
    graph G = graph2DMesh(n);
    startTime();
    int *Colors = colorGraph(G, 0);
    stopTime(.1,"Color Graph (2d grid)");
    G.del(); free(Colors);
  }

  reportTime("Color Graph (weighted average)");

}
