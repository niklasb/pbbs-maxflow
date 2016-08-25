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

#include <iostream>
#include <algorithm>
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include "IO.h"
#include "parseCommandLine.h"
#include <math.h>
#include "sequence.h"
#include "quickSort.h"
#include "Diameter.h"
#include <string>


using namespace std;
using namespace benchIO;


void readYahoo(intT* offsets,uint*edges,char* fname,char* f2name,intT& n,intT& m
	       ,char* idx_data, char* adj_data){
  ifstream adj(fname,ios::in|ios::binary);
  ifstream idx(f2name,ios::in|ios::binary);
  if(!adj || !idx) {cout <<"no file\n";abort();}
  else cout<<"reading yahoo graph\n";
  idx.seekg(0, ios::end);
  long idx_size = idx.tellg();
  idx.seekg(0);

  adj.seekg(0, ios::end);
  long adj_size = adj.tellg();
  adj.seekg(0);
  n=idx_size/sizeof(intT);
  m = adj_size/sizeof(uint);
  //cout<<n<<" "<<m<<endl;
  idx_data = (char *) malloc(idx_size);
  adj_data = (char *) malloc(adj_size);
  idx.read(idx_data,idx_size);
  adj.read(adj_data,adj_size);
  offsets = (intT*) idx_data;
  edges = (uint*) adj_data;
 
  adj.close(); idx.close();

}

graph readGraphFromFile(char* fname, 
			bool symmetric,
			bool binary,
			bool remDups, char* f2name=NULL){
  intT* offsets; uint* edges; char* idx_data; char* adj_data;
  intT n; intT m;
  if(f2name) {
    ifstream adj(fname,ios::in|ios::binary);
    ifstream idx(f2name,ios::in|ios::binary);
    if(!adj || !idx) {cout <<"no file\n";abort();}
    else cout<<"reading yahoo graph\n";
    idx.seekg(0, ios::end);
    long idx_size = idx.tellg();
    idx.seekg(0);

    adj.seekg(0, ios::end);
    long adj_size = adj.tellg();
    adj.seekg(0);
    n=idx_size/sizeof(intT);
    m = adj_size/sizeof(uint);
    //cout<<n<<" "<<m<<endl;
    idx_data = (char *) malloc(idx_size);
    adj_data = (char *) malloc(adj_size);
    idx.read(idx_data,idx_size);
    adj.read(adj_data,adj_size);
    offsets = (intT*) idx_data;
    edges = (uint*) adj_data;
 
    adj.close(); idx.close();

    //readYahoo(offsets,edges,fname,f2name,n,m,idx_data,adj_data);
  }
  else {
    if(binary){
      cout<<"binary\n";
      ifstream f (fname,ios::in|ios::binary);
      f.read((char*)&n,sizeof(intT));
      f.read((char*)&m,sizeof(intT));
      cout<<n<<" "<<m<<endl;
      offsets = newA(intT,n);
      edges = newA(uint,m);

      f.read((char*)offsets,n*sizeof(intT));
      f.read((char*)edges,m*sizeof(uint));


      f.close();
    } else {
      _seq<char> S = readStringFromFile(fname);
      words W = stringToWords(S.A, S.n);
      // if (W.Strings[0] != "AdjacencyGraph") {
      // 	cout << "Bad input file" << endl;
      // 	abort();
      // }
      // if (!strcmp(W.Strings[0],"AdjacencyGraph")) {
      // 	cout << "Bad input file" << endl;
      // 	abort();
      // }
      intT len = W.m -1;
      n = atol(W.Strings[1]);
      m = atol(W.Strings[2]);
 
      if (len != n + m + 2) {
	cout << "Bad input file length" << endl;
	abort();
      }

      offsets = newA(intT,n);
      edges = newA(uint,m);
      {parallel_for(intT i=0; i < n; i++) offsets[i] = atol(W.Strings[i+3]);}
      {parallel_for(intT i=0; i < m; i++) edges[i] = atoi(W.Strings[n+i+3]);}
      W.del();
    }
  }
  //cout<<"n = "<<n<<" m = "<<m<<endl;

  vertex *v = newA(vertex,n);
  
  parallel_for (intT i=0; i < n; i++) { 
    intT o = offsets[i];
    intT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].degree = l;
    v[i].Neighbors = edges+o;
  }

  if(remDups){
    //cout<<"removing duplicates\n";
    parallel_for(intT a=0;a<n;a++){
      uint d = v[a].degree;
      if(d>1)
  	quickSort(v[a].Neighbors,d,less<int>());
    }

    intT* newOffsets = newA(intT,m);
    bool* flags = newA(bool,m);
    memset(newOffsets,0,m*sizeof(intT));
    memset(flags,0,m);
    parallel_for(intT i=0;i<n;i++){
      uint d = v[i].degree;
      intT startIndex = v[i].Neighbors-edges;
      if(d > 0){
  	newOffsets[startIndex] = flags[startIndex] =
  	  (v[i].Neighbors[0] == i) ? 0 : 1;
  	for(uint j=1;j<d;j++){
  	  if((v[i].Neighbors[j] != i) && 
  	     (v[i].Neighbors[j] != v[i].Neighbors[j-1])){
  	    newOffsets[startIndex+j] = flags[startIndex+j] = 1; 
	    
  	  }
  	}
      }
    }
 
    sequence::plusScan(newOffsets,newOffsets,m);
    _seq<uint> Epacked = sequence::pack(edges,flags,m);
 
    free(flags);
    uint* edgesPacked = Epacked.A;

    //cout<<m<<" "<<Epacked.n<<endl;
    m = Epacked.n;

    { parallel_for(intT i=0;i<n;i++){
  	intT o = newOffsets[offsets[i]];
  	intT l = ((i == n-1) ? m : newOffsets[offsets[i+1]])-newOffsets[offsets[i]];
  	v[i].degree = l;
  	v[i].Neighbors = edgesPacked+o;
      }
    }
    free(newOffsets);
    free(edges);
    edges = edgesPacked;
  }
  free(offsets);
  return graph(v,n,m,edges);
  

}

void printGraph(graph A){
  intT n = A.n;
  for(intT i=0;i<n;i++){
    if(A.V[i].degree>0){
      cout<<i<<" : ";
      for(uint j=0;j<A.V[i].degree;j++)
	cout<<A.V[i].Neighbors[j]<<" ";
      cout<<endl;
    }
  }
}

void checkSymmetric(graph A){
  intT n = A.n;
  for(intT i=0;i<n;i++){
    for(uint j=0;j<A.V[i].degree;j++){
      uint ngh = A.V[i].Neighbors[j];
      if(ngh > i){
	bool b = false;
	for(uint k=0;k<A.V[ngh].degree;k++){
	  if(A.V[ngh].Neighbors[k] == i){
	    b = true;
	    break;
	  }
	}
	if(!b) {
	  cout<<"not symmetric between "<<i<<" and "<<ngh<<endl;
	  abort();
	}
      }
    }
  }
  cout<<"is symmetric"<<endl;
  abort();
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, "[-o <outFile>] [-symmetric] [-binary] [-remDups] [-2file] [-f function]");
  char* iFile;
  char* iFile2;
  char* oFile = P.getOptionValue("-o");
  bool symmetric = P.getOption("-symmetric");
  bool binary = P.getOption("-binary");
  bool remDups = P.getOption("-remDups");
  bool twoFiles = P.getOption("-2file");
  char* function = P.getOptionValue("-f");
  char* K = P.getOptionValue("-k");

  cout << "Diameter Estimation\n";
  graph A(NULL,0,0,NULL);
  if(!twoFiles){
    iFile = P.getArgument(0);
    cout<<"reading file\n";
    A = readGraphFromFile(iFile,symmetric,binary,remDups);
  } else {
    iFile = P.getArgument(1);
    iFile2 = P.getArgument(0);
    A = readGraphFromFile(iFile,symmetric,binary,remDups,iFile2);

  }
  cout<<"n = "<<A.n<<" m = " << A.m<<endl;
  //printGraph(A);

  int k;
  if(!K) k = 64; else k = atoi(K);
  int f;
  if(!function) f = 0; else f = atoi(function);
  
  startTime();
  int* lastChangedTime = computeDiameter(A,f,k);
  nextTime();
  if(oFile && (oFile != iFile)) {
    //quickSort(lastChangedTime,A.n,less<int>());
    writeIntArrayToFile(lastChangedTime,A.n,oFile);
  }
  free(lastChangedTime);

  A.del();



 }
