#include <iostream>
#include <iomanip>
#include "gettime.h"
#include "utils.h"
#include "graphGen.h"
#include "cilk.h"
#include <math.h>
using namespace std;

int* separator(graph G);

typedef unsigned short coltype;

template <class ETYPE>
struct sparseRowMajorS {
  int numRows;
  int numCols;
  int nonZeros;
  int* Starts;
  coltype* ColIds;
  ETYPE* Values;
  void del() {free(Starts); free(ColIds); if (Values != NULL) free(Values);}
  sparseRowMajorS(int n, int m, int nz, int* S, coltype* C, ETYPE* V) :
    numRows(n), numCols(m), nonZeros(nz), 
    Starts(S), ColIds(C), Values(V) {}
};

// Represents a matrix as 3 parts : diagonal blocks, offdiagonals and
// diagonal elements.
template <class E>
struct blockM {
  int bsize;
  sparseRowMajorS<E> BM;
  sparseRowMajor<E> OM;
  E *D;
  int *DI;
  int diags;
  blockM(sparseRowMajorS<E> _BM, sparseRowMajor<E> _OM, E *_D, int *_DI,
	 int _diags, int _bsize) :
    BM(_BM), OM(_OM), D(_D), bsize(_bsize),  DI(_DI), diags(_diags) {}
  void del() {BM.del(); OM.del(); free(D); //free(DI);
  }
};

// Separates a symmetric matrix into the three parts
template <class E>
blockM<E> filterBlocks(sparseRowMajor<E> A, int blockSize) {
  int offsetB = 0;
  int *StartsB = newA(int,A.numRows+1);
  coltype *ColIdsB = newA(coltype,A.nonZeros);
  E *ValuesB = newA(E,A.nonZeros);

  int offsetO = 0;
  int *StartsO = newA(int,A.numRows+1);
  int *ColIdsO = newA(int,A.nonZeros);
  E *ValuesO = newA(E,A.nonZeros);

  int diags =0;
  E *D = newA(E,A.numRows);
  int *DI = NULL; //newA(int,A.numRows);
  for (int i=0; i < A.numRows; i++) D[i] = 0.0;

  for (int i=0; i < A.numRows; i++) {
    StartsB[i] = offsetB;
    StartsO[i] = offsetO;
    for (int j=A.Starts[i]; j < A.Starts[i+1]; j++) {
      int colId = A.ColIds[j];
      if (i == colId) {
	D[i] = A.Values[j];
	//D[diags] = A.Values[j];
	//DI[diags] = i;
	diags++;
      }
      else if ((i != colId) && (colId/blockSize == i/blockSize)) {
	if (colId > i) {
	  ValuesB[offsetB] = A.Values[j];
	  ColIdsB[offsetB] = colId - (i/blockSize)*blockSize;
	  offsetB++;
	}
      } else {
	ValuesO[offsetO] = A.Values[j];
	ColIdsO[offsetO] = colId;
	offsetO++;
      }
    }
  }
  StartsB[A.numRows] = offsetB;
  StartsO[A.numRows] = offsetO;

  sparseRowMajorS<E> MB = sparseRowMajorS<E>(A.numRows,A.numCols,offsetB,
					StartsB,ColIdsB,ValuesB);
  sparseRowMajor<E> MO = sparseRowMajor<E>(A.numRows,A.numCols,offsetO,
					StartsO,ColIdsO,ValuesO);

  //cout << "bigblock n = " << offsetB << endl; // " smallblock n = " << offsetBB << endl;

  return blockM<E>(MB,MO,D,DI,diags,blockSize);
}
    

// multiply a compresses sparse row matrix
template <class E, class FM, class FA>
void MvMult(sparseRowMajor<E> A, E* v, E* o, FM fm, FA fa) {
  int n = A.numRows;
  int* starts = A.Starts; int *cols = A.ColIds; E* vals = A.Values;
  cilk_for (int i=0; i < n; i++) {
    int s = starts[i];
    int e = starts[i+1];
    E sum = 0;
    int* cp = cols+s;
    int* ce = cols+e;
    E* vp = vals+s; 
    while (cp < ce-1) {
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
    }
    if (cp < ce) 
      sum += v[*cp]*(*vp);
    o[i] = sum;
  }
}

// multiply a compresses sparse row matrix (sequential)
template <class E, class FM, class FA>
void MvMultSeq(sparseRowMajor<E> A, E* v, E* o, FM fm, FA fa) {
  int n = A.numRows;
  int* starts = A.Starts; int *cols = A.ColIds; E* vals = A.Values;
  for (int i=0; i < n; i++) {
    int s = starts[i];
    int e = starts[i+1];
    E sum = 0;
    int* cp = cols+s;
    int* ce = cols+e;
    E* vp = vals+s; 
    while (cp < ce-1) {
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
      sum = fa(sum,fm(v[*cp],*vp));
      cp++; vp++;
    }
    if (cp < ce) 
      sum += v[*cp]*(*vp);
    o[i] = sum;
  }
}

// multiplies the diagonal blocks of blocked CSR matrix
// x is input vector and y is output vector
template <class E, class FM, class FA>
void MvMultBlockCR(blockM<E> M, E* x, E* y, FM fm, FA fa) {
  int blockSize = M.bsize;
  int n = M.OM.numRows;

  int blocks = 1 + (n-1)/blockSize;
  cilk_for (int ii=0; ii < blocks; ii++) {
    int bs = ii*blockSize;
    int be = (ii+1)*blockSize;
    if (n - bs < blockSize) be = n;

    // The out of block entries
    { int* starts = M.OM.Starts; int *cols = M.OM.ColIds; E* vals = M.OM.Values;

      for (int i=bs; i < be; i++) {
	int s = starts[i];
	int e = starts[i+1];
	E sum = 0.0;
	int* cp = cols+s;
	int* ce = cols+e;
	E* vp = vals+s; 
	while (cp < ce-1) {
	  sum = fa(sum,fm(x[*cp],*vp));
	  cp++; vp++;
	  sum = fa(sum,fm(x[*cp],*vp));
	  cp++; vp++;
	}
	if (cp < ce) 
	  sum += x[*cp]*(*vp);
	y[i] = sum;
      }
    }

    // Commented out since test matrices don't have diagonal(needs to be fixed)
    // The diagonals
    //E* Diag = M.D;
    //for (int i=bs; i < be; i++) y[i] += fm(Diag[i],x[i]);

    // The block entries
    { int* starts = M.BM.Starts; coltype *cols = M.BM.ColIds; E* vals = M.BM.Values;
      E *xp = x + bs; // x offset for block
      E *yp = y + bs; // y offset for block

      for (int i=bs; i < be; i++) {
	E xi = x[i];
	E sum = 0;
	coltype *cp = cols + starts[i];
	coltype *ce = cols + starts[i+1];
	E* vp = vals + starts[i];

	while (cp < ce-1) { // one level unrolling
	  int j2 = *cp++;
	  E Aij = *vp++;
	  sum = fa(sum,fm(xp[j2],Aij));
	  E tmp = fa(yp[j2],fm(xi,Aij));

	  int j1 = *cp++;
	  Aij = *vp++;
	  yp[j2] = tmp; // Delayed 
	  sum = fa(sum,fm(xp[j1],Aij));
	  yp[j1] = fa(yp[j1],fm(xi,Aij));
	} 
	if (cp < ce) {
	  int j = *cp;
	  E Aij = *vp;
	  sum = fa(sum,fm(xp[j],Aij));
	  yp[j] = fa(yp[j],fm(xi,Aij));
	} 

	y[i] = fa(y[i],sum);
      }
    }
  }
}

template <class E>
void matrixStats(sparseRowMajor<E> A, int blockSize) {
  int minR = A.numRows; int maxR = 0;
  int minC = A.numCols; int maxC = 0;
  double logs = 0;
  int offDiag = 0;
  int diag = 0;
  for (int i=0; i < A.numRows; i++) {
    int s = A.Starts[i];
    int e = A.Starts[i+1];
    if (e-s > maxR) maxR = e-s;
    if (e-s < minR) minR = e-s;
    for (int j=s; j<e; j++) {
      int colId = A.ColIds[j];
      if (colId == i) diag++;
      if (i/blockSize != colId/blockSize) offDiag++;
      if (colId > maxC) maxC = colId;
      if (colId < minC) minC = colId;
      logs += log((double) abs(i - colId)+1);
    }
  }
  cout << setprecision(3) 
       << "nz=" << (A.nonZeros/1e6) << "M"
       << " minC=" << minR 
       << " maxC=" << maxR 
       << " avgC=" << ((float) A.nonZeros)/A.numRows
       << " diag=" << 100.0*diag/((float) A.numRows)
       << " logL=" << logs/(A.nonZeros*log(2.0)) 
       << " offD=" << 100.0*offDiag/((float) A.nonZeros)
       << endl;
}
    
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
void timeTest(sparseRowMajorD MD, int blockSize, string str) {
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

  // test correctness
  MvMult(M,v,o,mult<VT>(),add<VT>());
  checkSum(o,n,nz);

  // parallel
  startTime();
  MvMult(M,v,o,mult<VT>(),add<VT>());
  cout << str;  
  nextTime(" (par, all)");

  // sequential
  startTime();
  MvMultSeq(M,v,o,mult<VT>(),add<VT>());
  cout << str;  
  nextTime(" (seq, all)");

  // create and test blocked matrix
  blockM<VT> BM = filterBlocks(M,blockSize);
  MvMultBlockCR(BM,v,o,mult<VT>(),add<VT>());
  checkSum(o,n,nz);

  // time blocked matrix
  startTime();
  tmr.start();
  MvMultBlockCR(BM,v,o,mult<VT>(),add<VT>());
  cout << str;  
  nextTime(" (par, sym)");

  free(v); free(o); BM.del();
}

void timeTestSingDoub(sparseRowMajorD M, string str) {
  int blockCount = 16;
  int blockSize = M.numRows/blockCount;
  while (blockSize > (1<<16)) {
    blockCount = 2* blockCount;
    blockSize = M.numRows/blockCount;
  }
  //cout << "blocksize = " << blockSize << endl;


  //matrixStats(M, blockSize);
  if (M.Values != NULL) free(M.Values);
  M.Values = NULL;
  //timeTest<float>(M, blockSize, str+"(single)");
  timeTest<double>(M, blockSize, str+"(double)");
  M.del();
}

void timeFile(char* s, int reorder) {
  cout << endl;
  cout << s+5 << " : ";
  graph G = graphFromCsrFile(s,1);
  if (reorder) {
    int* I = separator(G);
    G = graphReorder(G,I);
    free(I);
  }
  sparseRowMajorD M = sparseFromGraph(G);
  G.del();
  timeTestSingDoub(M,"");
}

void timeFileMtx(char* s) {
  cout << s << " : ";
  timeTestSingDoub(sparseFromMtxFile(s,0),"");
}

void runFiles() {
  timeFile((char*) "data/Schenk_AFE/af_shell10.csrb",1);
  timeFile((char*) "data/Schenk_AFE/af_shell10.csrb",0);
  timeFile((char*) "data/Oberwolfach/bone010.csrb",1);
  timeFile((char*) "data/Oberwolfach/bone010.csrb",0);
  timeFile((char*) "data/Schmid/thermal2.csrb",1);
  timeFile((char*) "data/Schmid/thermal2.csrb",0);
  timeFile((char*) "/data/sc10datasets/thermal2.o2.csrb",0);
  timeFile((char*) "data/Schenk/nlpkkt80.csrb",1);
  timeFile((char*) "data/Schenk/nlpkkt80.csrb",0);
  //timeFile((char*) "data/vanHeukelum/cage15.csrb",1);
  timeFile((char*) "data/vanHeukelum/cage14.csrb",1);
  timeFile((char*) "data/vanHeukelum/cage14.csrb",0);
  timeFile((char*) "data/TSOPF/TSOPF_RS_b2383.csrb",1);
  timeFile((char*) "data/TSOPF/TSOPF_RS_b2383.csrb",0);
  timeFile((char*) "data/PARSEC/Ga41As41H72.csrb",1);
  timeFile((char*) "data/PARSEC/Ga41As41H72.csrb",0);
  timeFile((char*) "data/ND/nd24k.csrb",1);
  timeFile((char*) "data/ND/nd24k.csrb",0);
  timeFile((char*) "data/Rajat/rajat31.csrb",1);
  timeFile((char*) "data/Rajat/rajat31.csrb",0);
  timeFile((char*) "data/Zaoui/kkt_power.csrb",1);
  timeFile((char*) "data/Zaoui/kkt_power.csrb",0);
  timeFile((char*) "data/Freescale/Freescale1.csrb",1);
  timeFile((char*) "data/Freescale/Freescale1.csrb",0);
  timeFile((char*) "data/GHS_psdef/audikw_1.csrb",1);
  timeFile((char*) "data/GHS_psdef/audikw_1.csrb",0);
}

int cilk_main(int argc, char *argv[]) {
  int n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;
  timeTestSingDoub(sparseRandomWithDimension(2, 20, n),
		   "sparse VM (rand dim d=2, m = 20n) ");

  //runFiles();
}
