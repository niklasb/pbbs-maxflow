#ifndef __STACKSPACE__
#define __STACKSPACE__
#include <iostream>
#include <vector>
#include "cilk.h"
using namespace std;

struct stackSpace {
  long size;
  char* start;
  char* bottom;
  char* top;
  vector<char*> stack;
  long roundTo64(long n) { return (((n-1)>>6)+1)<<6;}

  void clearData() {
    cilk_for (long i=0; i < size; i++) start[i] = 0;
  }

  stackSpace(long n) : size(n) {
    start = (char*) malloc(n);
    clearData();
    bottom = top = (char*) roundTo64((long) start);
  }

  ~stackSpace() {
    size = 0;
    free(start);
  }

  bool isEmpty() { return bottom==top;}

  void clear() {
    top = bottom;
    stack.clear();
  }

  char* push(long n) {
    char* loc = top;
    stack.push_back(loc);
    top += roundTo64(n);
    if (loc - start > size) {
      cout << "stackSpace: overflow" << endl;
      abort();
    }
    return loc;
  }

  // pop top n blocks off top of stack
  void pop(intT n) {
    intT s = stack.size();
    if (s-n < 0) {
      cout << "stackSpace: underflow" << endl;
      abort();
    }
    top = stack[s-n];
    for (intT i=0; i < n; i++) stack.pop_back();
  }

  void pmemcpy(char* from, char* to, long n) {
    if (((long) from & 3) == 0 && 
	((long) to & 3) == 0  && 
	(n & 3) == 0) {
      intT* ifrom = (intT*) from;
      intT* ito = (intT*) to;
      cilk_for (long i = 0; i < n/4; i++) ito[i] = ifrom[i];
    } else {
      cilk_for (long i = 0; i < n; i++) to[i] = from[i];
    }
  }

  // pop n elements from stack which are k down
  // if k=0 then this is the same as pop
  void popKeepTop(intT n, intT k) {
    long s = stack.size();
    if (s-n-k < 0) {
      cout << "stackPace: underflow" << endl;
      abort();
    }
    char* to = stack[s-n-k];
    char* from =  stack[s-k];
    pmemcpy(from, to, top - from);
    for (intT i=1; i < k; i++) 
      stack[s-n-k+i] = stack[s-k+i] - (from - to);
    for (intT i=0; i < n; i++) stack.pop_back();
    top = top - (from - to);
  }

  char* ith(intT n) { return stack[stack.size()-1-n];}

  char* popKeepTop(intT n) {
    popKeepTop(n, 1);
    return ith(0);
  }
};

#endif
