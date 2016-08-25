#include "ansv.h"
#include "sequence.h"
#include "utils.h"

inline int getDepth(int i) {
  int a=0;
  int b=i-1;
  while (b > 0) {b = b >> 1; a++;}
  return a+1;
}

#define LEFT(i) ((i) << 1)
#define RIGHT(i) (((i) << 1) | 1)
#define PARENT(i) ((i) >> 1)

inline int getLeft(int **table, int depth, int n, int index) {
	int value = table[0][index];
	if (value == table[depth - 1][0]) return -1;

	int cur = PARENT(index), d, dist = 2;
	for (d = 1; d < depth; d++) {
		if ((cur + 1) * dist > index + 1) cur --;
		if (cur < 0) return -1;

		if (table[d][cur] >= value) cur = PARENT(cur);
		else break;

		dist <<= 1;
	}

	for ( ; d > 0; d--) {
		if (table[d - 1][RIGHT(cur)] < value) cur = RIGHT(cur);
		else cur = LEFT(cur);
	}
	return cur;
} 

inline int getRight(int **table, int depth, int n, int index) {
	int value = table[0][index];
	if (value == table[depth - 1][0]) return -1;

	int cur = PARENT(index), d, dist = 2;
	for (d = 1; d < depth; d++) {
		if (cur * dist < index) cur ++;
		if (cur * dist >= n) return -1;

		if (table[d][cur] >= value) cur = PARENT(cur);
		else break;

		dist <<= 1;
	}

	for ( ; d > 0; d--) {
		if (table[d - 1][LEFT(cur)] < value) cur = LEFT(cur);
		else cur = RIGHT(cur);
	}
	return cur;
}

void ComputeANSV(int *a, int n, int *left, int *right) {
	int depth = getDepth(n);
	int *all = new int[n];
	int **table = new int*[depth];
	//cout<<"there\n";
	table[0] = a;
	table[1] = all;
	for (int i = 2; i < depth; i++) {
		table[i] = table[i - 1] + (1 << (depth - i));
	}
	//cout<<"here\n";
	int m = n;
	for (int d = 1; d < depth; d++) {
		int m2 = m / 2;
		//cout<<d<<" "<<flush;
		cilk_for (int i = 0; i < m2; i++) {
			table[d][i] = min(table[d - 1][LEFT(i)], table[d - 1][RIGHT(i)]);
		}

		if (m % 2) {
			table[d][m2] = table[d - 1][LEFT(m2)];
		}

		m = (m + 1) / 2;
	}

	cilk_for (int i = 0; i < n; i++) {
		left[i] = getLeft(table, depth, n, i);
		right[i] = getRight(table, depth, n, i);
	}
	delete table;
	delete all;
}

//constructor
ANSV::ANSV(int* _a, int _n){
  a = _a;
  n = _n;
  tree = computeTree();
  leftNeighbors = computeANSVleft();
  rightNeighbors = computeANSVright(); 
};

pair<uint,uint>** ANSV::computeTree(){
  depth = utils::logUp(n)+1;
  pair<uint,uint>** tree = new pair<uint,uint>*[depth];
  pair<uint,uint>* leaves = new pair<uint,uint>[n];
  cilk_for(int i=0;i<n;i++) { leaves[i].first = a[i]; leaves[i].second = i; }

  tree[0] = leaves;
  int currentLevel = 1;
  int levelSize=n/2;
  int oddElt=n%2;
      
  pair<uint,uint>** tree2=tree; //cilk bug

  while(levelSize > 0){
    pair<uint,uint>* current = new pair<uint,uint>[levelSize+oddElt];
    int previousLevel = currentLevel-1;
    cilk_for(int j=0;j<levelSize;j++){
      if(tree2[previousLevel][2*j].first < tree2[previousLevel][2*j+1].first){
	current[j].first = tree2[previousLevel][2*j].first;
	current[j].second = tree2[previousLevel][2*j].second;
      }
      else {
	current[j].first = tree2[previousLevel][2*j+1].first;
	current[j].second = tree2[previousLevel][2*j+1].second;
      }
    }   
    if(oddElt) {
      current[levelSize].first = tree2[previousLevel][2*levelSize].first;
      current[levelSize].second = tree2[previousLevel][2*levelSize].second;
    }
     
    tree2[currentLevel]=current;
    int tmp = oddElt;
    oddElt = (levelSize+oddElt)%2;
    levelSize = (levelSize+tmp)/2;
    currentLevel++;
  }
  return tree;
}


int* ANSV::computeANSVleft(){
  //if going uphill to the left, start at the peak of the hill
  int* flags = new int[n];
  flags[0] = 0;
  cilk_for(int i=1;i<n;i++){
    if(a[i] > a[i-1]) flags[i] = i;
    else flags[i] = -1;
  }
  sequence::scanI(flags,flags,n,utils::maxF<int>(),(int) -1);

  int depth = ceil(log2(n));
  //indices of the left smaller value; -1 if none
  int* leftNeighbors = new int[n];
  leftNeighbors[0] = -1;
  cilk_for(int i=1;i<n;i++){
    int data = tree[0][i].first;
    int level = 0;
    int index = flags[i];
    
    if(depth > 1) {level++; index/=2;} //go to parent
    while(level < depth){
      if((tree[level][index].first < data) && 
	 (tree[level-1][index*2].first < data) && 
	 (tree[level-1][index*2].second < i)) break;
      index/=2;
      level++;
    }
    //going down tree
    if(level) {level--; index*=2;} //going to left child
    if(tree[level][index].second < i && tree[level][index].first < data){ 
      //node we are looking for is to the left
      while(level){ //not at leaves 
	if(tree[level-1][2*index+1].first < data){ index=2*index+1; } 
	//go to right child
	else { index*=2; } //go to left child
	level--;
      }
      leftNeighbors[i]=index;
    }
    else { leftNeighbors[i] = -1; }
  }
  int* copy = leftNeighbors; //cilk bug
  return copy;
}

int* ANSV::computeANSVright(){

  //if going uphill to the right, start at the peak of the hill
  int* flags = new int[n];
  flags[0] = 0;
  cilk_for(int i=0;i<n-1;i++){
    if(a[i+1] < a[i]) flags[n-1-i] = n-1-i;
    else flags[n-1-i] = -1;
  }
  sequence::scanI(flags,flags,n,utils::maxF<int>(),(int) -1);

  int depth = ceil(log2(n));
  //indices of the right smaller values; -1 if none
  int* rightNeighbors = new int[n]; 
  rightNeighbors[n-1]=-1;
  cilk_for(int i=0;i<n-1;i++){
    int data = tree[0][i].first;
    int level = 0;
    int index = n-1-flags[n-1-i];
    
    if(depth > 1) {level++; index/=2;} //go to parent
    while(level < depth){
      if((tree[level][index].first < data) && 
	 (tree[level-1][index*2+1].first < data) && 
	 (tree[level-1][index*2+1].second > i)) break;
      index/=2;
      level++;
    }
    //going down tree
    if(level) {level--; index=index*2+1;} //going to right child
    if(tree[level][index].second > i && tree[level][index].first < data){ 
      //node we are looking for is to the right
      while(level){ //not at leaves 
	if(tree[level-1][2*index].first < data){ index*=2; } 
	//go to left child
	else { index=index*2+1; } //go to right child
	level--;
      }
      rightNeighbors[i]=index;
    }
    else { rightNeighbors[i] = -1; }
  }
  int* copy = rightNeighbors; //cilk bug
  return copy;
}

int* ANSV::getLeftNeighbors(){
  return leftNeighbors;
}

int* ANSV::getRightNeighbors(){
  return rightNeighbors;
}

ANSV::~ANSV(){
  cilk_for(int i=0;i<depth;i++){
    delete[] tree[i];
  }
  delete[] tree;
}

