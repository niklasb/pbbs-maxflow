#include <iostream>
#include "cilk.h"
#include "gettime.h"
using namespace std;

void printMatrix(uint** M, int n, int m){
  for(int i=0;i<n;i++){
    for(int j=0;j<m;j++) cout<<M[i][j]<<" ";
    cout << endl;
  }
}

int getIndex(int n, int m, int i, int k){
  int maxLen = (n < m) ? n : m;
  int larger = (n < m) ? m : n;
  int offset;
  if (k <= maxLen) offset = k*(k+1)/2;
  else if(k > larger) offset = n*m - (n+m-k)*(n+m-k-1)/2;
  else offset = maxLen*(maxLen-1)/2 + (k+1-maxLen)*maxLen;
  return offset + i;
}

int editDistanceCacheEfficient(char* A, int n, char* B, int m){
  n++; m++;
  startTime();
  uint* M = new uint[n*m];
  for(int i=0;i<n;i++) M[getIndex(n,m,i,i)] = i;
  for(int j=1;j<m;j++) M[getIndex(n,m,0,j)] = j;
  /*for(int i=0;i<n;i++){
    for(int j=0;j<m;j++){
      cout<<M[getIndex(n,m,i,i+j)]<< " ";
    }
    cout<<endl;
    }*/
  cout<<endl;
  for(int k=2;k<n+m-1;k++){
    int start = (k<=m) ? 1 : k-m+1;
    int end = (k < n) ? k : n;
    if(k < n) end = k;
    else if(k < m) end = n-1;
    else end = n-1-(k+1-m);
    for(int i=1;i<end;i++){
      int j = k-i;
      if(A[i-1] == B[j-1]) M[getIndex(n,m,i,k)] = M[getIndex(n,m,i-1,k-2)];
      else {
	int a = M[getIndex(n,m,i-1,k-1)];
	int b = M[getIndex(n,m,i,k-1)];
	M[getIndex(n,m,i,k)] = 1 + ((a < b) ? a : b);
      }
    }
  }
    /*for(int i=0;i<n;i++){
      for(int j=0;j<m;j++){
	cout<<M[getIndex(n,m,i,i+j)]<< " ";
      }
      cout<<endl;
      }*/
  return M[n*m-1];
}

int editDistance(char* A, int n, char* B, int m){
  n++; m++;
  startTime();
  uint** M = new uint*[n];
  /*cilk_*/for(int i=0;i<n;i++) M[i] = new uint[m];
  /*cilk_*/for(int i=0;i<n;i++) M[i][0] = i;
  /*cilk_*/for(int j=0;j<m;j++) M[0][j] = j;
  for(int k=2;k<n+m-1;k++){
    int start = (k<=m) ? 1 : k-m+1;
    int end = (k < n) ? k : n;
    cilk_for(int i=start;i<end;i++){
      int j=k-i;
      if(A[i-1] == B[j-1]) M[i][j] = M[i-1][j-1];
      else M[i][j] = 1 + ((M[i-1][j] < M[i][j-1]) ? 
			  M[i-1][j] : M[i][j-1]);
    }
  }
  return M[n-1][m-1];
}

int editDistanceLinearSpace(char* A, int n, char* B, int m){
  n++; m++;
  if(n > m) { swap(n,m); char* tmp = A; A = B; B = tmp; }
  //n <= m
  uint* diag1 = new uint[n];
  uint* diag2 = new uint[n];
  uint* newDiag = new uint[n];
  diag1[0] = 0; diag2[0] = diag2[1] = 1;
  for(int k = 2; k < m; k++){
    newDiag[0] = k;
    if(k < n) newDiag[k] = k;
    int end = (k < n) ? k : n;
    cilk_for(int i=1;i<end;i++){
      if(A[i-1] == B[k-i-1]) newDiag[i] = diag1[i-1];
      else newDiag[i] = 1 + ((diag2[i-1] < diag2[i]) ? 
			     diag2[i-1] : diag2[i]);
    }
    //for(int i=0;i<n;i++) cout<<newDiag[i]<<" ";cout<<" newDiag k="<<k<<endl; 
    uint* tmp = diag1; diag1 = diag2; diag2 = newDiag; newDiag = tmp;
  }
  for(int k = m; k < n+m-1;k++){
    cilk_for(int i=0;i<n-1-(k-m);i++){
      if(A[i+k-m] == B[m-i-2]) newDiag[i] = diag1[i+(k>m)];
      else newDiag[i] = 1 + ((diag2[i] < diag2[i+1]) ?
			     diag2[i] : diag2[i+1]);
    }
    //for(int i=0;i<n;i++) cout<<newDiag[i]<<" ";cout<<" newDiag k="<<k<<" "<<(k>m)<<" "<<n-1-(k-m)<<endl;
    uint* tmp = diag1; diag1 = diag2; diag2 = newDiag; newDiag = tmp;
  }
  return diag2[0];
}

