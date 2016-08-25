// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "gettime.h"
#include "parallel.h"
#include "sequenceIO.h"
#include "parseCommandLine.h"
#include "sequence.h"
#include <algorithm>
using namespace std;
using namespace benchIO;

template <class ET, class HASH, class intT>
void timedict(ET* A, long n, HASH hashF, intT nIn, intT nDel) {
  intT nSe = n - nIn - nDel;
  cout<<"num inserts = "<<nIn<<" , num deletes = "<<nDel
      <<" , num finds = "<<nSe<<endl;

  // hashInt<int> foo;
  // intT nn = 10;
  // Table<hashInt<int>,intT> T0(nn, foo,2);
  // {parallel_for(intT i=0;i<nn;i++){T0.insert(i);}}
  // //T0.print();
  // cout<<T0.count()<<endl;
 
  // int* temp2 = newA(int,nn);
  //  {parallel_for(intT i=0;i<nn;i++)temp2[i]=T0.find(i);}

  //  int count = 0;
  //  {for(intT i=0;i<nn;i++)if(temp2[i] != -1) count++; else cout <<i<<" not found\n";}
  //  cout<<"num found = "<<count<<endl;

  //  //{parallel_for(intT i=0;i<nn;i++)T0.insert(i);}

  //  //T0.print();

  //  _seq<intT> s = T0.entries();
  // //for(intT i=0;i<s.n;i++)cout<<s.A[i]<<" ";cout<<endl;
  //  cout<<T0.count()<<endl;
  
  // {parallel_for(intT i=0;i<nn;i++)T0.deleteVal((i*2)%nn);}
  // //T0.print();
  // cout<<T0.count()<<endl;
  // {parallel_for(intT i=0;i<nn;i++)temp2[i]=T0.find(i);}

  // count = 0;
  // {for(intT i=0;i<nn;i++)if(temp2[i] != -1) count++;}
  // cout<<"num found = "<<count<<endl;

  // T0.clear();
  // cout<<T0.count()<<endl;
  // int* temp = newA(int,nn);
  // {parallel_for(intT i=0;i<nn;i++)temp[i]=T0.find(i);}
  // count = 0;
  //  {for(intT i=0;i<nn;i++)if(temp[i] != -1) count++;}
  //  cout<<"num found = "<<count<<endl;

  //  return;

  // _tm.stop();
  // //get things into cache
  // intT* C = newA(intT,n);
  // C[n-1] = 0;
  // {parallel_for(intT i=0;i<n-1;i++) {
  //     if(A[i] == A[i+1]) C[i] = 1; else C[i] = 0;}}
  // int bar = sequence::plusScan(C,C,(intT)n);
  // free(C);
  // T.clear();
  // _tm.start();
  if(0){
    Table<hashInt<intT>,intT> TT(nIn, hashInt<intT>(), 1);
    startTime();
    for(int j=0;j<39;j++) {
      int size = (1<<utils::log2Up(nIn))-1;
      int chunkSize = size/40;
      cout<<"load = "<<(j*2.5+1.25)/100<<"\n";
      cout<<"numops = "<<chunkSize<<endl;

      {parallel_for(intT i = j*chunkSize; i < (j+1)*chunkSize; i++) { 
	  TT.insert(i);}}
      nextTime("insert");
      {parallel_for(intT i = j*chunkSize; i < (j+1)*chunkSize; i++) { 
	  TT.find(i);}}
      nextTime("find inserted");
      {parallel_for(intT i = j*chunkSize; i < (j+1)*chunkSize; i++) { 
	  TT.deleteVal(i);}}
      nextTime("delete inserted");
      //insert back in
      {parallel_for(intT i = j*chunkSize; i < (j+1)*chunkSize; i++) { 
	  TT.insert(i);}}
      nextTime("insert back in");
      // cout<<TT.count()<<endl;
      // nextTime("count");

      cout<<endl;
    }
    exit(0);
  }

  for(int r=0;r<2;r++){
    startTime();
    Table<HASH,intT> T(nIn, hashF, 2);
    //bool* B = newA(bool,nSe);
    nextTime("initialize");

    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.insert(A[i]);}}
    nextTime("inserts");
    T.entries();
    nextTime("elements");
    {parallel_for(intT i = 0; i < nSe; i++) { 
	T.find(hashF.getKey(A[i + nIn + nDel]));}}
    nextTime("find random");
    {parallel_for(intT i = 0; i < nDel; i++) { 
	T.deleteVal(hashF.getKey(A[i+nIn]));}}
    nextTime("delete random");

    T.clear();
    nextTime("clear table");
    cout<<"all operations below are on "<<nIn<<" elements\n";
    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.find(hashF.getKey(A[i]));}}
    nextTime("find on empty table");
    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.deleteVal(hashF.getKey(A[i]));}}
    nextTime("delete on empty table");
    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.insert(A[i]);}}
    nextTime("inserts");
    _tm.stop();
    cout<<T.count()<<endl;
    _tm.start();
    T.entries();
    nextTime("elements");
    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.find(hashF.getKey(A[i]));}}
    nextTime("find inserted");
    // _tm.stop();
    // cout<<sequence::plusReduce(B,nIn)<<endl;
    // _tm.start();
    {parallel_for(intT i = 0; i < nIn; i++) { 
	T.deleteVal(hashF.getKey(A[i]));}}
    nextTime("delete inserted");
    _tm.stop();
    cout<<T.count()<<endl;
    T.del();
    //free(B);

    //random writes
    // intT* D = newA(intT,nIn);
    // {parallel_for(intT i=0;i<nIn;i++) D[i] = utils::hash(i) % nIn;}
    // intT* E = newA(intT,nIn);
    // _tm.start();
    // {parallel_for(intT i=0;i<nIn;i++) E[D[i]] = i;}
    // nextTime("random writes");
    // _tm.stop();

    // //random write first
    // {parallel_for(intT i=0;i<nIn;i++) E[i] = INT_MAX;}
    // _tm.start();
    // {parallel_for(intT i=0;i<nIn;i++) if(E[D[i]] == INT_MAX) E[D[i]] = i;} 
    // nextTime("random write if empty"); 
    // free(D); free(E);

  }
  // parallel_for(intT i=0; i < nSe; i++)
  //   F[i] = B[i] != hashF.empty();
  // _seq<ET> R = sequence::pack(B, F, nSe);
  // free(B);
  // free(F);
  //return R;
}

typedef pair<intT,intT> intPair;


int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
 
  char* iFile = argv[1];
  
  seqData D = readSequenceFromFile(iFile);
  int dt = D.dt;
  
  intT numInserts = (argc < 3) ? D.n/3 : atoi(argv[2]);
  intT numDeletes = (argc < 4) ? D.n/3 : atoi(argv[3]);


  switch (dt) {
  case intType: {
    intT* A = (intT*) D.A;
    timedict(A, D.n, hashInt<intT>(), numInserts, numDeletes);

    delete A;}
    break;

  case intPairT: {
    intPair* A = (intPair*) D.A;
    timedict(A,D.n,hashSimplePair(),numInserts,numDeletes);
  }
    break;

   case stringT: {
     char** A = (char**) D.A;
     timedict(A, D.n, hashStr(), numInserts, numDeletes);

   }
     break;

   case stringIntPairT: {
     stringIntPair* AA = (stringIntPair*) D.A;
     stringIntPair** A = new stringIntPair*[D.n];
     parallel_for (intT i=0; i < D.n; i++) A[i] = AA+i;
     timedict(A, D.n, hashPair<hashStr,intT>(hashStr()),
				       numInserts, numDeletes);
     delete A;
   }
     break;

  default:
    cout << "removeDuplicates: input file not of right type" << endl;
    return(1);
  }
}
