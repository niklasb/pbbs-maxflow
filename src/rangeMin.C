#include "rangeMin.h"
#include "math.h"
#include "utils.h"

#define BSIZE 16

myRMQ::myRMQ(int* _a, int _n){
  a = _a;
  n = _n;
  m = 1 + (n-1)/BSIZE;
  precomputeQueries();
};

void myRMQ::precomputeQueries(){
  depth = log2(m) + 1;
  table = new int*[depth];
  cilk_for(int i=0;i<depth;i++) {
    table[i] = new int[n];
  }

  cilk_for(int i=0; i < m; i++) {
    int start = i*BSIZE;
    int end = min(start+BSIZE,n);
    int k = i*BSIZE;
    for (int j = start+1; j < end; j++) 
      if (a[j] < a[k]) k = j;
    table[0][i] = k;
  }
  int dist = 1;
  for(int j=1;j<depth;j++) {
    cilk_for(int i=0; i< m-dist; i++){
      if (a[table[j-1][i]] <= a[table[j-1][i+dist]])
	table[j][i] = table[j-1][i];
      else table[j][i] = table[j-1][i+dist];
    }
    cilk_for(int i = m-dist; i<m; i++) {
      table[j][i] = table[j-1][i];
    }
    dist*=2;
  }

}

int myRMQ::query(int i, int j){
  //same block
  if (j-i < BSIZE) {
    int r = i;
    for (int k = i+1; k <= j; k++) 
      if (a[k] < a[r]) r = k;
    return r;
  } 
  int block_i = i/BSIZE;
  int block_j = j/BSIZE;
  int min = i;
  for(int k=i+1;k<(block_i+1)*BSIZE;k++){
    if(a[k] < a[min]) min = k;
  }
  for(int k=j; k>=(block_j)*BSIZE;k--){
    if(a[k] < a[min]) min = k;
  }
  if(block_j == block_i + 1) return min;
  int outOfBlockMin;
  //not same or adjacent blocks
  if(block_j > block_i + 1){
    block_i++;
    block_j--;
    if(block_j == block_i) outOfBlockMin = table[0][block_i];
    else if(block_j == block_i + 1) outOfBlockMin = table[1][block_i];
    else {
      int k = log2(block_j - block_i);
      int p = 1<<k; //2^k
      outOfBlockMin = a[table[k][block_i]] <= a[table[k][block_j+1-p]]
	? table[k][block_i] : table[k][block_j+1-p];
    }
  }

  return a[min] < a[outOfBlockMin] ? min : outOfBlockMin;

}

myRMQ::~myRMQ(){
  
  cilk_for(int i=0;i<depth;i++){
    delete[] table[i];
  }
  delete[] table;
}


