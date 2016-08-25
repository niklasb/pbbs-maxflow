// -*- C++ -*-

#include "gettime.h"
#define OMPSAFE 1
#include "sequence.h"
#include "utils.h"
#include "cilk.h"
#include <iostream>
#include <ctype.h>
#include <math.h>

#define CHECK 0
#define TTYPE double
#define full 0

template <class T>
void check(bool cond, int i, T val, string s) {
  if (!cond) { 
    cout << "Error for " << s << ": at i=" << i << " v=" << val << endl; 
    abort(); } 
}

template <class T>
void check2(bool cond, int i, int j, T val, string s) {
  if (!cond) { 
    cout << "Error for " << s << ": at i=" << i << " j=" << j << 
      " v=" << val << endl; 
    abort(); } 
}

template <class ET, class F> 
void scanS(ET* In, ET* Out, int n, F f, ET zero) {
  ET s = zero;
  int i;
  for (i=0; i < n-1; i++) {
    ET t = In[i];
    Out[i] = s;
    s = f(s,t);
  }
  Out[i] = s;
}

template <class ETYPE>
struct odd { int operator() (ETYPE& i) {return ((int) i) & 1;} };

struct CtoI { 
  char* A; 
  CtoI(char* AA) : A(AA) {}
  int operator() (char i) {return (int) A[i];} 
};

int cilk_main(int argc, char* argv[]) {
  int n = 10;
  if (argc > 1) n = std::atoi(argv[1]);
  long t1;

   {
     // make sure mem is allocated
     TTYPE *i1 = newA(TTYPE,n);
     TTYPE *i2 = newA(TTYPE,n);
     TTYPE *i3 = newA(TTYPE,n);
     cilk_for(int i =0; i < n; i++) i1[i] = 0;
     cilk_for(int i =0; i < n; i++) i2[i] = 0;
     cilk_for(int i =0; i < n; i++) i3[i] = 0;
     free(i1); free(i2); free(i3);
   }

  {
    TTYPE *o1 = newA(TTYPE,n);
    cilk_for(int i =0; i < n; i++) o1[i] = 0;
    startTime();
    cilk_for(int i =0; i < n; i++) o1[i] = i;
    nextTime("tabulate (double)");
    free(o1);
  }

  {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    cilk_for (int i =0; i < n; i++) out[i] = in[i]+1; 
    nextTime("map (double, +1)");
    free(in); free(out);
  }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    int sqrtn = (int) floor(sqrt((double) n));
    startTime();
    int k = 0;
    for (int j=0; j < n; j += sqrtn) 
      cilk_for (int i = j; i < min(n,j + sqrtn); i++) 
	out[i] = in[i]+1; 
    nextTime("map sqrt (double, +1)");
    free(in); free(out);
  }

  if (full) {
    char* in = newA(char,n);
    char* out = newA(char,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    cilk_for (int i =0; i < n; i++) out[i] = isalpha(in[i]);
    nextTime("map (char, isalpha)");
    free(in); free(out);
  }

  {
    TTYPE* in = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    startTime();
    TTYPE v = sequence::reduce(in,n,utils::addF<TTYPE>());
    nextTime("reduce (double, add)");
    free(in);
  }

//   {
//     char* in = newA(char,n);
//     cilk_for (int i =0; i < n; i++) in[i] = 1;
//     startTime();
//     int s2 = sequence::reduce<int>(0,n,utils::addF<int>(),CtoI(in));
//     nextTime("map reduce (char, add)");
//     free(in);
//   }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    startTime();
    int v = sequence::maxIndex(in,n,greater<TTYPE>());
    nextTime("max Index (double)");
    if (v != n-1) { 
      cout << "Error for max Index, returned index " << v 
	   << " instead of n-1" << endl;
      abort(); } 
    free(in);
  }

  // {
  //   bool* in = newA(bool,n);
  //   int* out = newA(int,n);
  //   cilk_for (int i =0; i < n; i++) in[i] = 1;
  //   startTime();
  //   int v = sequence::enumerate(in,out,n);
  //   nextTime("enumerate");
  //   free(in);
  //   free(out);
  // }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    startTime();
    int v = sequence::scan(in,in,n,utils::addF<TTYPE>(),(TTYPE) 0);
    nextTime("scan (double, add, inplace)");
    if (CHECK) 
      for (int i=0; i < n; i++) 
	check(((TTYPE) i)==in[i],i,in[i],"scan inplace");
    free(in);
  }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    startTime();
    int v = sequence::scanBack(in,in,n,utils::addF<TTYPE>(),(TTYPE) 0);
    nextTime("scan back (double, add, inplace)");
    if (CHECK) 
      for (int i=0; i < n; i++) 
	check(((TTYPE) i)==in[n-i-1],i,in[n-i-1],"back scan inplace");
    free(in);
  }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    int v = sequence::scan(in,out,n,utils::addF<TTYPE>(),(TTYPE) 0);
    nextTime("scan (double, add)");
    if (CHECK) 
      for (int i=0; i < n; i++) 
	check(((TTYPE) i)==out[i],i,out[i],"scan");
    free(in); free(out);
  }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    int v = sequence::scanI(in,out,n,utils::addF<TTYPE>(),(TTYPE) 0);
    nextTime("scanI (double, add)");
    if (CHECK) 
      for (int i=0; i < n; i++) 
	check(((TTYPE) i)==out[i]-1,i,out[i],"scan");
    free(in); free(out);
  }

  if (full) {
    int* in = newA(int,n);
    int* out = newA(int,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    int v = sequence::scan(in,out,n,utils::maxF<int>(),(int) -1);
    nextTime("scan (int, max)");
    if (CHECK) 
      for (int i=0; i < n; i++) 
	check(((int) i)==out[i]+1,i,out[i],"scan");
    free(in); free(out);
  }

  if (full) {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    scanS(in,out,n,utils::addF<TTYPE>(),(TTYPE) 0);
    nextTime("sequential scan (double, add)");
    free(in); free(out);
  }

  if (full) {
    int* in = newA(int,n);
    int* out = newA(int,n);
    cilk_for (int i =0; i < n; i++) in[i] = 1;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    scanS(in,out,n,utils::maxF<int>(),(int) 0);
    nextTime("sequential scan (int, max)");
    free(in); free(out);
  }

  if (full)   {
    TTYPE* in = newA(TTYPE,n);
    TTYPE* out = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) in[i] = i;
    cilk_for (int i =0; i < n; i++) out[i] = 0;
    startTime();
    int m = sequence::filter(in,out,n,odd<TTYPE>());
    nextTime("filter (double, odd)");
    free(in); free(out);
  }

  if (full) {
    bool* f1 = newA(bool,n);
    TTYPE* i1 = newA(TTYPE,n);
    TTYPE* o1 = newA(TTYPE,n);
    cilk_for (int i =0; i < n; i++) {
      f1[i] = i%2;
      i1[i] = i;
    }
    cilk_for (int i =0; i < n; i++) o1[i] = 0;
    startTime();
    int m = sequence::pack(i1,o1,f1,n);
    nextTime("pack (double, odd)");
    if (CHECK) {
      if (m != n/2) abort();
      for (int i=0; i < m; i++) 
	check((2*i+1)==o1[i],i,o1[i],"pack");
    }
    free(f1); free(i1); free(o1);
  }

  if (full) {
    int* A = new int[n];
    int* B = new int[n];
    int* I = new int[n];
    cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
    cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
    timeStatement(cilk_for (int i=0;i<n;i++) B[i] = A[I[i]],"gather (int)");
    free(A); free(B); free(I);
  }

  {
    TTYPE* A = new TTYPE[n];
    TTYPE* B = new TTYPE[n];
    int* I = new int[n];
    cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
    cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
    timeStatement(cilk_for (int i=0;i<n;i++) B[i] = A[I[i]],"gather (double)");
    free(A); free(B); free(I);
  }

  if (full) {
    int* A = new int[n];
    int* B = new int[n];
    int* I = new int[n];
    cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
    cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
    timeStatement(cilk_for (int i=0;i<n;i++) B[I[i]] = A[i],"scatter (int)");
    free(A); free(B); free(I);
  }

  {
    TTYPE* A = new TTYPE[n];
    TTYPE* B = new TTYPE[n];
    int* I = new int[n];
    cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
    cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
    timeStatement(cilk_for (int i=0;i<n;i++) B[I[i]] = A[i],"scatter (double)");
    if (full) {
      cilk_for (int i=0;i<n;i++)  I[i] = I[i]%1000000;
      timeStatement(cilk_for (int i=0;i<n;i++) 
		    B[I[i]] = A[i],"scatter contended (10^6)");
      cilk_for (int i=0;i<n;i++)  I[i] = I[i]%100000;
      timeStatement(cilk_for (int i=0;i<n;i++) 
		    B[I[i]] = A[i],"scatter contended (10^5)");    
    }
    free(A); free(B); free(I);
  }

  if (full) {
    int* A = new int[n];
    int* B = new int[n];
    int* I = new int[n];
    cilk_for (int i =0; i < n; i++) A[i] = B[i] = 0;
    cilk_for (int i=0; i < n; i++) I[i] = utils::hash(i)%n;
    timeStatement(cilk_for (int i=0;i<n;i++) 
		  utils::CAS(&B[I[i]],0,A[i]),"scatter CAS (int)");
    cilk_for (int i=0;i<n;i++)  I[i] = I[i]%1000000;
    timeStatement(cilk_for (int i=0;i<n;i++) 
		  utils::CAS(&B[I[i]],0,A[i]),"scatter CAS contended (10^6)");
    timeStatement(cilk_for (int i=0;i<n;i++) 
		    if (B[I[i]] == 0) 
		      utils::CAS(&B[I[i]],0,A[i]);,
		  "scatter CAS contended conditional (10^6)");
    cilk_for (int i=0;i<n;i++)  I[i] = I[i]%100000;
    timeStatement(cilk_for (int i=0;i<n;i++) 
		    utils::CAS(&B[I[i]],0,A[i]),
		  "scatter CAS contended (10^5)");
    timeStatement(cilk_for (int i=0;i<n;i++) 
		    if (B[I[i]] == 0) 
		      utils::CAS(&B[I[i]],0,A[i]);,
		  "scatter CAS contended conditional (10^5)");
    free(A); free(B); free(I);
  }

  if (full) {
      TTYPE* a = new TTYPE[n];
      for (int i=0; i < n; i++) a[i] = i;
      startTime();
      int j;
      std::partition(a,a+n,odd<TTYPE>());
      nextTime("partition");
    }

  // Tests that are not currently used
  if (0) {
    {
      startTime();
      int **A = newA(int*,n/10);
      {cilk_for (int i=0; i < n/10; i++) {
	  A[i] = (int*) malloc(sizeof(int)*10);
	  A[i][0] = 0; // make sure it is touched
	}}
      nextTime("malloc (n/10 blocks of 10 ints)");
      {cilk_for (int i=0; i < n/10; i++) free(A[i]);}
    }

    {
      startTime();
      int **A = newA(int*,n/10);
      {cilk_for (int i=0; i < n/10; i++) {
	  A[i] = (int*) malloc(sizeof(int)*10);
	  A[i][0] = 0; // make sure it is touched
	}}
      nextTime("malloc (n/10 blocks of 10 ints)");
      {cilk_for (int i=0; i < n/10; i++) free(A[i]);}
    }

    {
      startTime();
      int **A = newA(int*,n);
      {cilk_for (int i=0; i < n; i++) {
	  A[i] = (int*) malloc(sizeof(int));
	}}
      nextTime("malloc (n blocks of 1 ints)");
      {cilk_for (int i=0; i < n; i++) free(A[i]);}
    }

  }
}

