#include <iostream>
#include <cstdlib>
#include "sequence.h"
#include "graphGen.h"
#include "gettime.h"
#include "cilk.h"
#include "graph.h"
#include "seq.h"
using namespace std;

int* maxIndependentSet(graph G, int seed);
void checkMaximalIndependentSet(graph G, int* Flags);

void checkSymmetric(graph Gr){
  int n = Gr.n;
  vertex * G = Gr.V;
  seq<vindex> vertices = seq<vindex>(n, utils::identityF<int>());

  for(int i=0;i<vertices.size();i++){
    vindex v = vertices[i];
    for(int j=0;j<G[v].degree;j++){
      vindex ngh = G[v].Neighbors[j];
      bool ok = 0;
      for(int k=0;k<G[ngh].degree;k++){
	vindex ngh_ngh = G[ngh].Neighbors[k];
	if(ngh_ngh == v) ok = 1;
      }
      if(!ok) {
	cout<<"not symmetric between vertices " << v << " and " << ngh << endl;
      }
    }
  }

}

// **************************************************************
//    MAIN
// **************************************************************

int cilk_main(int argc, char *argv[]) {
  int n,m;
  if (argc > 1) n = atoi(argv[1]); else n = 10;
  if (argc > 2) m = atoi(argv[2]); else m = 10;

  {
    graph G = graphRandomWithDimension(2, m, n);
    checkSymmetric(G);
    cout << "m = " << G.m << endl;
    G = graphReorder(G,NULL);
    startTime();
    int *flags = maxIndependentSet(G,0);
    stopTime(.1,"Maximal Independent Set (rand dim=2, m=n*10)");
    checkMaximalIndependentSet(G,flags);
    G.del(); free(flags);
  }

  {
    graph G = graph2DMesh(n);
    G = graphReorder(G,NULL);
    startTime();
    int *flags = maxIndependentSet(G,0);
    stopTime(.2,"Maximal Independent Set (2d grid)");
    checkMaximalIndependentSet(G,flags);
    G.del(); free(flags);
  }

  {
    edgeArray E = edgeRmat(n,5*n,0,.5,.1,.1,.3);
    graph G = graphFromEdges(E,1);
    E.del();
    //cout<<"before shuffle"<<endl;
    //checkSymmetric(G);

    G = graphReorder(G,NULL);
    //check symmetric
    //cout<<"after shuffle"<<endl;
    //checkSymmetric(G);

    startTime();
    int *flags = maxIndependentSet(G,0);
    stopTime(.1,"Maximal Independent Set (rMat m = n*10)");
    checkMaximalIndependentSet(G,flags);
    G.del(); free(flags);
  }

  reportTime("Maximal Independent Set (weighted average)");

}

