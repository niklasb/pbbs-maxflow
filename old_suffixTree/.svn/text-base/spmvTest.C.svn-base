#include <iostream>
#include <iomanip>
#include "gettime.h"
#include "utils.h"
#include "graphGen.h"
#include "spmv.h"
#include "cilk.h"
#include <math.h>
using namespace std;

template <class VT>
struct mult {  VT operator() (VT a, VT b) {return a*b;} };

template <class VT>
struct add {  VT operator() (VT a, VT b) {return a+b;} };

template <class VT>
void checkSum(VT* X, int n, int nz) {
    int sum = 0;
    for (int i=0; i < n; i++) sum += (int) X[i];
    if (sum != nz) cout << "bad result, total = " << sum 
			<< " size = " << nz << endl;
}

template <class VT>
void timeTest(sparseRowMajorD MD, string str) {
  //cout << "nonzeros=" << MD.nonZeros << endl;
  double t;
  timer tmr;
  int rounds = 10;
  int n = MD.numRows; int m = MD.numCols; int nz = MD.nonZeros;

  // put values in
  VT* Vals = newA(VT,nz);
  cilk_for(int i=0; i < MD.nonZeros; i++) Vals[i] = 1.0;
  sparseRowMajor<VT> M = sparseRowMajor<VT>(n,m,nz,MD.Starts,MD.ColIds,Vals);

  // create source and dest vectors
  VT * v = newA(VT,M.numRows);
  VT * o = newA(VT,M.numCols);
  for (int i=0; i < M.numRows; i++) v[i] = 1.0;
  for (int i=0; i < M.numCols; i++) o[i] = 0.0;

  startTime();
  MvMult(M,v,o,mult<VT>(),add<VT>());
  cout << str;  
  stopTime(.1,"");
  checkSum(o,n,nz);

  free(v); free(o); MD.del(); free(Vals);
}

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;

  {
    sparseRowMajorD M = sparseRandomWithDimension(2, 20, n);
    timeTest<double>(M, "sparse VM (rand dim d=2, m = n*20) ");
  }

  {
    edgeArray E = edgeRmat(n,20*n,0,.55,.05,.05,.35);
    graph G = graphFromEdges(E,0);
    E.del();
    sparseRowMajorD M = sparseFromGraph(G);
    G.del();
    timeTest<double>(M, "sparse VM (rMat m = n*20) ");
  }

  reportTime("sparse VM (weighted average)");
}
