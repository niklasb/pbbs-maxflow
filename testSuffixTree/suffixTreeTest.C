// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch, Julian Shun and the PBBS team
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
#include "gettime.h"
#include "stringGen.h"
#include "suffixTree.h"
#include <fstream>
#include <string.h>
#include "utils.h"
using namespace std;

bool isPermutation(uintT *SA, uintT n) {
  bool *seen = new bool[n];
  for (uintT i = 0;  i < n;  i++) seen[i] = 0;
  for (uintT i = 0;  i < n;  i++) seen[SA[i]] = 1;
  for (uintT i = 0;  i < n;  i++) if (!seen[i]) return 0;
  return 1;
}

bool sleq(uintT *s1, uintT *s2) {
  if (s1[0] < s2[0]) return 1;
  if (s1[0] > s2[0]) return 0;
  return sleq(s1+1, s2+1);
} 

bool isSorted(uintT *SA, uintT *s, uintT n) {
  for (uintT i = 0;  i < n-1;  i++) {
    if (!sleq(s+SA[i], s+SA[i+1])) {
      cout << "not sorted at i = " << i+1 << " : " << SA[i] 
	   << "," << SA[i+1] << endl;
      return 0;
    }
  }
  return 1;  
}

//check if string appears in s
bool containsString(uintT* s, uintT* string, uintT n,uintT strLength){
  for(uintT i=0;i<= n-strLength;i++){
    for(uintT j=0;j<strLength;j++){
      if(string[j] == 0) return true;
      else if (string[j] != s[i+j]) break;
    }
  }
  return false;
}

uintT computeLCP(uintT *s1, uintT *s2, uintT matches){
  if(s1[0]==s2[0]) return computeLCP(s1+1,s2+1,matches+1);
  else return matches;
}

bool isLCPcorrect(uintT *SA, uintT *LCP, uintT *s, uintT n){
  for(uintT i=0;i<n-1;i++){
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
  if(argc < 2) {
    cout<<"Usage: ./suffixTreeTest <filename> [-s <search string>] [-c]\n";
    cout<<"To search for a string in the suffix tree pass the flag \"-s\" followed by the search string. To search for n/20 random substrings from the input file in the suffix tree, pass the flag \"-c\". For integer files, pass the flag \"-i\".\n";
  }
  else {
    char* filename = (char*)argv[1];
    long n;
    uintT* str;
    if(utils::getOption(argc,argv,(char*)"-i")){
      pair<uintT*,uintT> s = dataGen::readIntFile(filename);
      str = s.first; n = s.second;
    }
    else {
      _seq<char> s = dataGen::readCharFile(filename);
      n = s.n;
      str = newA(uintT,n+3);
      cilk_for (uintT i = 0; i < n; i++) str[i] = (unsigned char) s.A[i];
      str[n] = 0; str[n+1] = 0; str[n+2] = 0;
      s.del();
    }
    cout<<filename<<endl;
    for(int r=0;r<3;r++){
      startTime();

      pair<uintT*,uintT*> SA_LCP = suffixArray(str, n, true);
      uintT* SA = SA_LCP.first;
      uintT* LCP = SA_LCP.second;
      //nextTime("Time for suffix array with input as a file");
      suffixTree T = suffixArrayToTree(SA,LCP,n,str);
      //nextTime("Time for converting to suffix tree");
      if(!T.n) continue;
      char* searchString;
      if(searchString = utils::getOptionValue(argc,argv,(char*)"-s")){      
	uintT* string = new uintT[strlen(searchString)+1];
	string[strlen(searchString)] = 0;
	for(long i=0;i<strlen(searchString);i++) 
	  string[i]=(unsigned char) searchString[i];
	uintT result = T.search(string);
	if(UINT_T_MAX == result) cout << "\"" << searchString << "\" not found\n";
	else cout << "\"" << searchString << "\" found at offset "<<result<<endl;
      }
      if(utils::getOption(argc,argv,(char*)"-c")){
	//generate 500,000 substrings and 500,000 random substrings
	//of lengths uniformly distributed between 1 and 50 
	uint m = 1000000;
	uint MAX_STR_LEN = 50;
	uintT* strings = new uintT[(1+MAX_STR_LEN)*m];
	uintT* offsets = new uintT[m];
	cilk_for(long i=0;i<m/2;i++){
	  uintT strlen = min(1 + i/20000,n);
	  intT k = dataGen::hash<intT>(i)%(1+n-strlen);
	  long j;
	  for(j=0;j<strlen;j++)
	    strings[(MAX_STR_LEN+1)*i+j] = str[k+j];
	  strings[(MAX_STR_LEN+1)*i+j] = 0;
	}
	// for(int i=0;i<MAX_STR_LEN+1;i++)cout<<strings[i]<<" ";cout<<endl;
	// for(int i=0;i<MAX_STR_LEN+1;i++)cout<<strings[MAX_STR_LEN+1+i]<<" ";cout<<endl;
	// cout<<T.search(strings+MAX_STR_LEN+1)<<endl;
	// cout<<strings[MAX_STR_LEN+1]<<endl;
	
	cilk_for(long i=0;i<m/2;i++){
	  long strlen = 1 + i/20000;
	  long j;
	  for(j=0;j<strlen;j++)
	    strings[(MAX_STR_LEN+1)*(i+m/2)+j] = dataGen::hash<uintT>(i);
	  strings[(MAX_STR_LEN+1)*(i+m/2)+j] = 0;	  
	}
	//nextTime("Generate queries");
	timer queryTime;
	queryTime.start();
	cilk_for(long i=0;i<m;i++){
	  uintT* s = strings + (MAX_STR_LEN+1)*i;
	  offsets[i] = T.search(s);
	}
	queryTime.stop();
	queryTime.reportTotal("time to search 1,000,000 strings");
	//nextTime("Time to search 1,000,000 strings");
	for (long i=0; i<m/2; i++) {
	  uintT* s = strings + (MAX_STR_LEN+1)*i;
	  if (offsets[i] == UINT_T_MAX) {
	    cout << "not found ith string : " << i << endl;
	    abort();
	  } else {
	    uintT* s2 = str + offsets[i];
	    for (long j=0; s[j] != 0; j++)
	      if (s[j] != s2[j]) {
		cout << "bad match at i=" << i << ": found location " << offsets[i]
		     << " does not match string at " 
		     << dataGen::hash<uintT>(i)%n << endl;
		for (long k=0; k < MAX_STR_LEN; k++) cout << s[k] << ":";
		cout << endl;
		for (long k=0; k < MAX_STR_LEN; k++) cout << s2[k] << ":";
		cout << endl;
		abort();
	      }
	  }
	}

    // 	// generate n/STR_LEN random offsets and copy into strings
    // 	int STR_LEN = 20;
    // 	intT m = n/STR_LEN;
    // 	intT* strings = new intT[(STR_LEN+1)*m];
    // 	intT* offsets = new intT[m];
    // 	cilk_for (intT i=0;i<m;i++) {
    // 	  intT k = dataGen::hash<intT>(i)%n;
    // 	  intT j = 0;
    // 	  for (j=0; j<STR_LEN && k+j < n; j++) 
    // 	    strings[(STR_LEN+1)*i+j] = str[k+j];
    // 	  strings[(STR_LEN+1)*i+j] = 0;
    // 	}
    // 	startTime();

    // 	cilk_for (intT i=0; i<m; i++) { 
    // 	  intT* s = strings + (STR_LEN+1)*i;
    // 	  offsets[i] = T.search(s);
    // 	}
    // 	nextTime("Time to search n/20 strings");

    // 	for (intT i=0; i<m; i++) {
    // 	  intT matches = 0;
    // 	  intT* s = strings + (STR_LEN+1)*i;
    // 	  if (offsets[i] == -1) {
    // 	    cout << "not found ith string : " << i << endl;
    // 	    abort();
    // 	  } else {
    // 	    intT* s2 = str + offsets[i];
    // 	    for (intT j=0; s[j] != 0; j++)
    // 	      if (s[j] != s2[j]) {
    // 		cout << "bad match at i=" << i << ": found location " << offsets[i]
    // 		     << " does not match string at " 
    // 		     << dataGen::hash<intT>(i)%n << endl;
    // 		for (intT k=0; k < STR_LEN; k++) cout << s[k] << ":";
    // 		cout << endl;
    // 		for (intT k=0; k < STR_LEN; k++) cout << s2[k] << ":";
    // 		cout << endl;
    // 		abort();
    // 	      }
    // 	  }
    // 	}
      }
      T.del();
    }
    cout<<endl;
  }
}
