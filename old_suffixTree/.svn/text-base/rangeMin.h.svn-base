#ifndef _myRMQ_hpp_
#define _myRMQ_hpp_

#include <iostream>
#include "cilk.h"
using namespace std;

class myRMQ{
protected:
  int* a;
  int n;
  int m;
  int** table;
  int depth;

public:
  myRMQ(int* a, int n);
  void precomputeQueries();
  int query(int i, int j);
  ~myRMQ();
};

#endif
