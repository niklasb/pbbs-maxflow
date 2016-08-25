#include <iostream>
#include <limits>
#include "sequence.h"
#include "graphGen.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

pair<int,int> BFS_ND(vindex start, graph GA);
pair<int,int> BFS(vindex start, graph GA);

// **************************************************************
//    MAIN
// **************************************************************

int cilk_main(int argc, char *argv[]) {
  int t1, t2, n, m, dimension;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;
  if (argc > 2) m = std::atoi(argv[2]); else m = 10;
  if (argc > 3) dimension = std::atoi(argv[3]); else dimension = 2;

  if (0) {
    graph G = graphRandomWithDimension(dimension, m, n);
    startTime();
    pair<int,int> r = BFS_ND(0,G);
    nextTime("Nondeterministic BFS (rand dim=2, m=n*10)");
    utils::myAssert(r.first == G.n, "BFS did not visit all vertices");
    //cout << "visited = " << r.first << " rounds = " << r.second << endl;
    G.del();
  }

  {
    graph G = graphRandomWithDimension(dimension, m, n);
    startTime();
    pair<int,int> r = BFS(0,G);
    stopTime(.2, "BFS (return BFS tree) (rand dim=2, m=n*10)");
    utils::myAssert(r.first == G.n, "BFS did not visit all vertices");
    G.del();
  }

  {
    graph G = graph3DMesh(n);
    startTime();
    pair<int,int> r = BFS(0,G);
    stopTime(.1,"BFS (return BFS tree) (3d grid)");
    utils::myAssert(r.first == G.n, "BFS did not visit all vertices");
    G.del();
  }

  {
    edgeArray E = edgeRmat(n,5*n,0,.5,.1,.1,.3);
    graph G = graphFromEdges(E,1);
    E.del();
    startTime();
    pair<int,int> r = BFS(0,G);
    stopTime(.1,"BFS (return BFS tree) (rMat, .5,.1,.3)");
    //cout << "Visited: " << r.first << " vertices in "
    // << r.second << " levels"<< endl;
    G.del();
  }

  reportTime("BFS (weighted average)");

  //char* fname = (char*) "/home/guyb/data/graph/kkt_power.mtx";
  //char* fname = (char*) "/home/guyb/data/graph/cage15.mtx";
  //graph G = graphFromMtxFile(fname,1);
}
