#include <sstream>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "bytecode.h"
#include "quickSort.h"
#include "utils.h"
#include "graph.h"
#include "graphTesting.h"
#include "parseCommandLine.h"
using namespace std;

template<class F>
struct printAdjT {
  stringstream* ss;
  printAdjT(stringstream *_ss) : ss(_ss) {}
  bool srcTarg(F f, uintE src, uintE target, uintT edgeNumber) {
    *ss << target << endl;
    return true;
  }
  bool srcTarg(F f, uintE src, uintE target, intE weight, uintT edgeNumber) {
    *ss << target << endl;
    return true;
  }
};

void writeAdjGraph(graph<symmetricVertex> G, ofstream *of, bool weighted) {
  symmetricVertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    stringstream ss;
    char *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
    if(weighted)
      decodeWgh(printAdjT<printF>(&ss), printF(), nghArr, i, d);
    else
      decode(printAdjT<printF>(&ss), printF(), nghArr, i, d);
    *of << ss.str();
  }
}

//converts binary compressed graph to text format
//works for unweighted graphs
//decoding weighted graph gives back unweighted version of graph
//Warning: make sure thread count is set to 1 since we are writing to file
void readGraphFromBinary(char* fname, char* oFile, bool weighted) {
  ifstream in(fname,ifstream::in |ios::binary);
  in.seekg(0,ios::end);
  long size = in.tellg();
  in.seekg(0);
  cout << "size = " << size << endl;
  char* s = (char*) malloc(size);
  in.read(s,size);
  long* sizes = (long*) s;
  long n = sizes[0], m = sizes[1], totalSpace = sizes[2];

  cout << "n = "<<n<<" m = "<<m<<" totalSpace = "<<totalSpace<<endl;
  cout << "reading file..."<<endl;

  uintT* offsets = (uintT*) (s+3*sizeof(long));
  long skip = 3*sizeof(long) + (n+1)*sizeof(uintT);
  uintE* Degrees = (uintE*) (s+skip);
  skip+= n*sizeof(uintE);
  char* edges = s+skip;

  in.close();

  ofstream out(oFile,ofstream::out);
  out << "AdjacencyGraph\n" << n << endl << m << endl;
  cout<<"writing offsets..."<<endl;
  uintT* DegreesSum = newA(uintT,n);
  parallel_for(long i=0;i<n;i++) DegreesSum[i] = Degrees[i];
  sequence::plusScan(DegreesSum,DegreesSum,n);
  stringstream ss;
  for(long i=0;i<n;i++) ss << DegreesSum[i] << endl;
  free(DegreesSum);
  out << ss.str();
  graph<symmetricVertex> G(offsets,offsets,edges,edges,n,m,Degrees,Degrees,s);
  
  cout << "writing edges..."<<endl;

  writeAdjGraph(G,&out, weighted);

  out.close();
}

int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv,"[-w] <inFile> <outFile>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  bool weighted = P.getOption("-w");
  readGraphFromBinary(iFile,oFile,weighted);
}
