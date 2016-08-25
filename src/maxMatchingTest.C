#include <iostream>
#include <limits>
#include <limits>
#include "seq.h"
#include "gettime.h"
#include "cilk.h"
#include "graphGen.h"
using namespace std;

vindex* maximalMatching(graph G, int seed);
void checkMaximalMatching(graph GG, vindex* GD);

vindex* maximalMatchingT(seq<vindex> T);
void checkMaximalMatchingT(seq<vindex> T, vindex* GD);

// **************************************************************
//    MAIN
// **************************************************************

int cilk_main(int argc, char *argv[]) {
  int n, m, dimension;
  if (argc > 1) n = atoi(argv[1]); else n = 10;
  if (argc > 2) m = atoi(argv[2]); else m = 10;
  if (argc > 3) dimension = atoi(argv[3]); else dimension = 2;

  {
    // Tree Matching
    vindex* T = newA(vindex,n);
    cilk_for (int i=0; i < n; i++) {
      int j = 0;
      do T[i] = utils::hash(i+n*j++) % n; // avoid self pointers
      while (T[i] == i);
    }
    seq<vindex> TT = seq<vindex>(T,n);

    startTime();
    vindex* GD = maximalMatchingT(TT);
    stopTime(.1, "Maximal Matching (tree rand parent)");
    checkMaximalMatchingT(TT,GD);
    free(GD); free(T);
  }

  {
    graph G = graphRandomWithDimension(dimension, m, n);
    G = graphReorder(G,NULL);
    startTime();
    vindex* GD = maximalMatching(G,0);
    stopTime(.1, "Maximal Matching (rand dim=2, m=n*10)");
    checkMaximalMatching(G,GD);
    G.del();
    free(GD);
  }

  {
    graph G = graph2DMesh(n);
    G = graphReorder(G,NULL);
    startTime();
    vindex* GD = maximalMatching(G,0);
    stopTime(.1,"Maximal Matching (2d mesh)");
    checkMaximalMatching(G,GD);
    G.del();
    free(GD);
  }

  reportTime("Maximal Matching (weighted average)");

    //int *flags = newA(int,n);
    //cilk_for(int i=0; i < n; i++) flags[i] = (GD[i] < INT_MAX) ? 1 : 0;
    //int count = sequence::reduce(flags,n,utils::addF<int>());
    //free(flags);
    //cout << "set size = " << count << endl;
}
