#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "gettime.h"
#include "cilk.h"
#include <algorithm>
#include "manis.h"
using namespace std;

void manis(set* sets, int numSets, int numElts);

int cilk_main(int argc, char* argv[]) {

  int numSets,numEdges,numElts;
  if (argc > 1) numSets = atoi(argv[1]); else numSets = 10;
  if (argc > 2) numElts = atoi(argv[2]); else numElts = 10;
  if (argc > 3) numEdges = atoi(argv[3]); else numEdges = numSets*numElts/2;
  startTime();
  //generate random bipartite graph
  set* sets = edgeRandomBipartite(numSets,numEdges,numElts);
  nextTime("random bipartite graph");
  random_shuffle(sets,sets+numSets);
  nextTime("shuffle");
  /*
  for(int i=0;i<numSets;i++){
    set s = sets[i];
    for(int j=0;j<s.size;j++){
      cout<<s.Elts[j]<<" ";
    }
    cout<<endl;
    }
  */
  manis(sets,numSets,numElts);
  //only chosen sets remain in sets array
  nextTime("Manis time");
  free(sets);

}
