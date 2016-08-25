#include "gettime.h"
#include "utils.h"
#include "merge.h"
#include <iostream>
using namespace std;

#define TTYPE double

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
     TTYPE *s1 = newA(TTYPE,n/2);
     TTYPE *s2 = newA(TTYPE,n/2);
     TTYPE *s3 = newA(TTYPE,n);
     for (int i=0; i < n/2; i++) {
       s1[i] = 2*i;
       s2[i] = 2*i + 1;
     }

     startTime();
     merge(s1,n/2,s2,n/2,s3,std::less<TTYPE>());
     stopTime(.1, "Merging (interleaved)");

     for (int i=0; i < 2*(n/2); i++) 
       if (s3[i] != i) {
	 cout << "Merge: Error at i=" << i << " v=" << s3[i] << endl; 
	 abort();
       }

     free(s1);  free(s2);  free(s3);

   }

  reportTime("Merging (weighted average)");
}


