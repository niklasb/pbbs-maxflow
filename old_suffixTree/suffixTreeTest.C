#include <iostream>
#include <cstdlib>
#include "gettime.h"
#include "stringGen.h"
#include "suffixTree.h"
#include "math.h"
#include "stackSpace.h"
#include <fstream>
#include <vector>
using namespace std;

#define CHECK 0

bool isPermutation(intT *SA, intT n) {
  bool *seen = new bool[n];
  for (intT i = 0;  i < n;  i++) seen[i] = 0;
  for (intT i = 0;  i < n;  i++) seen[SA[i]] = 1;
  for (intT i = 0;  i < n;  i++) if (!seen[i]) return 0;
  return 1;
}

bool sleq(intT *s1, intT *s2) {
  if (s1[0] < s2[0]) return 1;
  if (s1[0] > s2[0]) return 0;
  return sleq(s1+1, s2+1);
} 

bool isSorted(intT *SA, intT *s, intT n) {
  for (intT i = 0;  i < n-1;  i++) {
    if (!sleq(s+SA[i], s+SA[i+1])) {
      cout << "not sorted at i = " << i+1 << " : " << SA[i] 
	   << "," << SA[i+1] << endl;
      return 0;
    }
  }
  return 1;  
}

//check if string appears in s
bool containsString(intT* s, intT* string, intT n,intT strLength){
  for(intT i=0;i<= n-strLength;i++){
    for(intT j=0;j<strLength;j++){
      if(string[j] == 0) return true;
      else if (string[j] != s[i+j]) break;
    }
  }
  return false;
}

intT computeLCP(intT *s1, intT *s2, intT matches){
  if(s1[0]==s2[0]) return computeLCP(s1+1,s2+1,matches+1);
  else return matches;
}

bool isLCPcorrect(intT *SA, intT *LCP, intT *s, intT n){
  for(intT i=0;i<n-1;i++){
    //compute LCP
    if(computeLCP(s+SA[i],s+SA[i+1],0) != LCP[i]){
      cout << "LCP incorrect at i=" << i << " : " << 
	"LCP in in fact: " << computeLCP(s+SA[i],s+SA[i+1],0) <<
	" , but algorithm returns LCP = " << LCP[i] << endl;
      return 0;
    }
  }
  return 1;
}

int cilk_main(int argc, char **argv) {
  if (argc >= 3){
    char* filename = (char*)argv[2];
    intT* str;
    intT n;
    if(argv[1][0] == 'f'){
      seq<char> s = dataGen::readCharFile(filename);
      n = s.size();
      str = newA(intT,n+1);
      for (intT i = 0; i < n; i++) str[i] = (unsigned char) s[i];
      str[n] = 0;
      s.del();
    }
    else if(argv[1][0] == 'i'){
      pair<intT*,intT> s = dataGen::readIntFile(filename);
      str = s.first;
      n = s.second;
    }
    
       
    stackSpace* stack = new stackSpace(1024+8*sizeof(intT)*(long) n);
    int rounds = 1;
    suffixTree T;

    startTime();
    for (int i=0; i < rounds; i++) {
      // run once to initialize memory
      pair<intT*,intT*> SA_LCP = suffixArray(str, n, true, stack);
      nextTime("Time for suffix array with input as a file");
      intT* SA = SA_LCP.first;
      if (CHECK) isPermutation(SA,n);
      intT* LCP = SA_LCP.second;
      if (CHECK) isLCPcorrect(SA, LCP, str, n);
      T = suffixArrayToTree(SA,LCP,n,str,stack);
      nextTime("Time for converting to suffix tree");
      if (i != rounds-1) {
	T.del();
	stack->clear();
      }
    }

    //generate 500,000 substrings and 500,000 random substrings
    //of lengths uniformly distributed between 1 and 50 
    int m = 1000000;
    int MAX_STR_LEN = 50;
    intT* strings = new intT[(1+MAX_STR_LEN)*m];
    intT* offsets = new intT[m];
    cilk_for(int i=0;i<m/2;i++){
      int strlen = 1 + i/20000;
      intT k = dataGen::hash<intT>(i)%(n-strlen);
      int j;
      for(j=0;j<strlen;j++)
    	strings[(MAX_STR_LEN+1)*i+j] = str[k+j];
      strings[(MAX_STR_LEN+1)*i+j] = 0;
    }
    cilk_for(int i=0;i<m/2;i++){
      int strlen = 1 + i/20000;
      int j;
      for(j=0;j<strlen;j++)
    	strings[(MAX_STR_LEN+1)*(i+m/2)+j] = dataGen::hash<intT>(i);
      strings[(MAX_STR_LEN+1)*(i+m/2)+j] = 0;	  
    }
    nextTime("Generate queries");
    cilk_for(int i=0;i<m;i++){
      intT* s = strings + (MAX_STR_LEN+1)*i;
      offsets[i] = T.search(s);
    }
    nextTime("Time to search 1,000,000 strings");
    for (int i=0; i<m/2; i++) {
      intT* s = strings + (MAX_STR_LEN+1)*i;
      if (offsets[i] == -1) {
    	cout << "not found ith string : " << i << endl;
    	abort();
      } else {
    	intT* s2 = str + offsets[i];
    	for (intT j=0; s[j] != 0; j++)
    	  if (s[j] != s2[j]) {
    	    cout << "bad match at i=" << i << ": found location " << offsets[i]
    		 << " does not match string at " 
    		 << dataGen::hash<intT>(i)%n << endl;
    	    for (int k=0; k < MAX_STR_LEN; k++) cout << s[k] << ":";
    	    cout << endl;
    	    for (int k=0; k < MAX_STR_LEN; k++) cout << s2[k] << ":";
    	    cout << endl;
    	    abort();
    	  }
      }
    }
    /*
    // generate n/STR_LEN random offsets and copy into strings
    int STR_LEN = 20;
    int m = n/STR_LEN;
    int* strings = new int[(STR_LEN+1)*m];
    int* offsets = new int[m];
    cilk_for (int i=0;i<m;i++) {
      int k = dataGen::hash<int>(i)%n;
      int j = 0;
      for (j=0; j<STR_LEN && k+j < n; j++) 
	strings[(STR_LEN+1)*i+j] = str[k+j];
      strings[(STR_LEN+1)*i+j] = 0;
    }
    nextTime("Generate random");
    
    cilk_for (int i=0; i<m; i++) { 
      int* s = strings + (STR_LEN+1)*i;
      offsets[i] = T.search(s);
    }
    nextTime("Time to search n/20 strings");

    for (int i=0; i<m; i++) {
      int matches = 0;
      int* s = strings + (STR_LEN+1)*i;
      if (offsets[i] == -1) {
	cout << "not found ith string : " << i << endl;
	abort();
      } else {
	int* s2 = str + offsets[i];
	for (int j=0; s[j] != 0; j++)
	  if (s[j] != s2[j]) {
	    cout << "bad match at i=" << i << ": found location " << offsets[i]
		 << " does not match string at " 
		 << dataGen::hash<int>(i)%n << endl;
	    for (int k=0; k < STR_LEN; k++) cout << s[k] << ":";
	    cout << endl;
	    for (int k=0; k < STR_LEN; k++) cout << s2[k] << ":";
	    cout << endl;
	    abort();
	  }
      }
    }
    */
    //delete [] strings; delete [] offsets;
    T.del();
  }
}
