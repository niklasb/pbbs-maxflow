#include "IO.h"
#include "parseCommandLine.h"
#include "graph.h"
#include "graphIO.h"
#include "graphUtils.h"
#include "parallel.h"
using namespace benchIO;
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"-o <outFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");

  edgeArray<intT> G = readEdgeArrayFromFile<intT>(iFile);
  writeGraphToFile<intT>(graphFromEdges(G,1),oFile);

}
