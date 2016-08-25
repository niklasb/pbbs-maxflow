#ifndef _myRMQ_hpp_
#define _myRMQ_hpp_

#include <iostream>
#include "cilk.h"
using namespace std;

class myRMQ{
protected:
  intT* a;
  intT n;
  intT m;
  intT** table;
  intT depth;

public:
  myRMQ(intT* a, intT n);
  void precomputeQueries();
  intT query(intT i, intT j);
  ~myRMQ();
};

#endif
