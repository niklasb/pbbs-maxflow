#ifndef A_TRANSPOSE_INCLUDED
#define A_TRANSPOSE_INCLUDED

#define _TRANS_THRESHHOLD 64

template <class E>
struct transpose {
  E *A, *B;
  transpose(E *AA, E *BB) : A(AA), B(BB) {}

  void transR(int rStart, int rCount, int rLength,
	      int cStart, int cCount, int cLength) {
    //cout << "cc,rc: " << cCount << "," << rCount << endl;
    if (cCount < _TRANS_THRESHHOLD && rCount < _TRANS_THRESHHOLD) {
      for (int i=rStart; i < rStart + rCount; i++) 
	for (int j=cStart; j < cStart + cCount; j++) 
	  B[j*cLength + i] = A[i*rLength + j];
    } else if (cCount > rCount) {
      int l1 = cCount/2;
      int l2 = cCount - cCount/2;
      cilk_spawn transR(rStart,rCount,rLength,cStart,l1,cLength);
      transR(rStart,rCount,rLength,cStart + l1,l2,cLength);
      cilk_sync;
    } else {
      int l1 = rCount/2;
      int l2 = rCount - rCount/2;
      cilk_spawn transR(rStart,l1,rLength,cStart,cCount,cLength);
      transR(rStart + l1,l2,rLength,cStart,cCount,cLength);
      cilk_sync;
    }	
  }

  void trans(int rCount, int cCount) {
    transR(0,rCount,cCount,0,cCount,rCount);
  }
};

template <class E>
struct blockTrans {
  E *A, *B;
  int *OA, *OB, *L;

  blockTrans(E *AA, E *BB, int *OOA, int *OOB, int *LL) 
    : A(AA), B(BB), OA(OOA), OB(OOB), L(LL) {}

  void transR(int rStart, int rCount, int rLength,
	     int cStart, int cCount, int cLength) {
    //cout << "cc,rc: " << cCount << "," << rCount << endl;
    if (cCount < _TRANS_THRESHHOLD && rCount < _TRANS_THRESHHOLD) {
      for (int i=rStart; i < rStart + rCount; i++) 
	for (int j=cStart; j < cStart + cCount; j++) {
	  E* pa = A+OA[i*rLength + j];
	  E* pb = B+OB[j*cLength + i];
	  int l = L[i*rLength + j];
	  //cout << "pa,pb,l: " << pa << "," << pb << "," << l << endl;
	  for (int k=0; k < l; k++) *(pb++) = *(pa++);
	}
    } else if (cCount > rCount) {
      int l1 = cCount/2;
      int l2 = cCount - cCount/2;
      cilk_spawn transR(rStart,rCount,rLength,cStart,l1,cLength);
      transR(rStart,rCount,rLength,cStart + l1,l2,cLength);
      cilk_sync;
    } else {
      int l1 = rCount/2;
      int l2 = rCount - rCount/2;
      cilk_spawn transR(rStart,l1,rLength,cStart,cCount,cLength);
      transR(rStart + l1,l2,rLength,cStart,cCount,cLength);
      cilk_sync;
    }	
  }
 
  void trans(int rCount, int cCount) {
    transR(0,rCount,cCount,0,cCount,rCount);
  }

} ;

#endif // A_TRANSPOSE_INCLUDED
