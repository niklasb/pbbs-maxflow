#include <iostream>
#include <limits>
#include <limits>
#include "seq.h"
#include "gettime.h"
#include "cilk.h"
#include "graphGen.h"
using namespace std;

vindex* maximalMatching(edgeArray EA, int numRows);
void checkMaximalMatching(vindex* vertices, int n, edgeArray EA);

int cilk_main(int argc,char*argv[]){

  int n, degree, dimension;
  if (argc > 1) n = atoi(argv[1]); else n = 10;
  if (argc > 2) degree = atoi(argv[2]); else degree = 10;
  if (argc > 3) dimension = atoi(argv[3]); else dimension = 2;

  {
    edgeArray EA = edgeRandomWithDimension(dimension,degree,n);
    random_shuffle(EA.E,EA.E+EA.nonZeros-1);
    cout<<"generating MM with n = "<<n<<", m = "<<EA.nonZeros<<endl;
    startTime();
    vindex* vertices = maximalMatching(EA,n);
    nextTime("Maximal matching on random graph:");
    cout<<"verifying maximal matching"<<endl;
    checkMaximalMatching(vertices,n,EA);
    EA.del();
    free(vertices);
  }

  {
    edgeArray EA = edge2DMesh(n);
    random_shuffle(EA.E,EA.E+EA.nonZeros-1);
    cout<<"generating MM with n = "<<n<<", m = "<<EA.nonZeros<<endl;
    startTime();
    vindex* vertices = maximalMatching(EA,n);
    nextTime("Maximal matching on 2D mesh:");
    cout<<"verifying maximal matching"<<endl;
    checkMaximalMatching(vertices,n,EA);
    EA.del();
    free(vertices);
  }

}
