#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include "IO.h"
#include "parseCommandLine.h"
#include "blockRadixSort.h"

using namespace std;
using namespace benchIO;

struct getuF {intT operator() (edge e) {return e.u;} };

edgeArray readEdgeArrayFromFile(char* fname) {
  _seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
  // if (W.Strings[0] != "EdgeArray") {
  //   cout << "Bad input file" << endl;
  //   abort();
  // }
  intT n = (W.m-1)/2;
  edge *E = newA(edge,2*n);
  {parallel_for(intT i=0; i < n; i++) {
      E[2*i] = edge(atol(W.Strings[2*i + 1]), 
			atol(W.Strings[2*i + 2]));
      E[2*i+1] = edge(atol(W.Strings[2*i+2]),atol(W.Strings[2*i+1]));
    }
  }
  W.del();
  intT maxR = 0;
  intT maxC = 0;
  for (intT i=0; i < 2*n; i++) {
    maxR = max<intT>(maxR, E[i].u);
    maxC = max<intT>(maxC, E[i].v);
  }
  return edgeArray(E, maxR+1, maxC+1, 2*n);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, " input output");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  
  ifstream file (iFile, ios::in | ios::binary);
  if (!file.is_open()) {
    std::cout << "Unable to open file: " << iFile << std::endl;
    abort();
  }
  edgeArray A = readEdgeArrayFromFile(iFile);

  intT m = A.nonZeros;
  intT n = A.numRows;

  intT* offsets = newA(intT,n+1);
  offsets[n] = m;
  intSort::iSort(A.E,offsets,m,n,getuF());
  uint *edges = newA(uint,m);
  parallel_for(intT i=0;i<m;i++) edges[i] = A.E[i].v;

  A.del();


  cout<<"n = " << n << " m = "<<m << endl;

  file.close();
  ofstream file2(oFile, ios::out | ios::binary);

  file2.write((char*)&n,sizeof(intT));

  file2.write((char*)&m,sizeof(intT));

  file2.write((char *)offsets,sizeof(intT)*(n));
  file2.write((char *)edges,sizeof(uint)*(m));

  file2.close();


  free(offsets); free(edges);

}
