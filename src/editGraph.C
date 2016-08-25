#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "sequence.h"
#include "radix.h"
#include "hash.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

#define timeDetail 0

// A structure that keeps a sequence of strings all allocated from
// the same block of memory
struct stringA {
  int n; // total number of characters
  char* Chars;  // array storing all strings
  int m; // number of substrings
  char** Strings; // pointers to strings (all should be null terminated)
  stringA() {}
  stringA(char* C, int nn, char** S, int mm)
    : Chars(C), n(nn), Strings(S), m(mm) {}
  void del() {free(Chars); free(Strings);}
  void printSome(int l) {
    for (int i=0; i < l && i < m; i++) {
      char *s = Strings[i];
      while (*s) cout << *s++;
      cout << endl;
    }
    cout << "***word count  = " << m << " char count = " << n << endl;
  }
};

struct wordStartF {
  char* _S;
  wordStartF(char* S) : _S(S) {} 
  bool operator() (int i) {return _S[i] & !_S[i-1];}
};

// this side effects input and uses it as part of stringA
stringA parseString(char *Str, int n) {
  char *F = newA(char,n);

  // Make lower case, clear all non alphabetic chars, and identify
  // all alphabetic chars by setting F[i]
  {
    cilk_for (int i=0; i < n; i++) {
      char c = Str[i];
      if ((c) >= 'a' && (c) <= 'z') F[i] = 1;
      else if ((c) >= 'A' && (c) <= 'Z') {Str[i] = Str[i]-32; F[i] = 1;}
      else {Str[i] = 0; F[i] = 0;}
    }
  }

  // get offsets of word starts 
  seq<int> offsetsS = seq<int>::packIndex(1,n-1,wordStartF(F));

  // add offsets to string start
  int m = offsetsS.size();
  char **Strings = newA(char*, m+1);
  { cilk_for (int i=0; i < m; i++) 
      Strings[i] = Str+offsetsS.S[i];}
  Strings[m] = Str+n;
  offsetsS.del(); free(F);
  return stringA(Str,n,Strings,m);
}

stringA packStrings(char** S, int m) {
  int *L = newA(int, m);
  { cilk_for(int i=0; i < m; i++) L[i] = strlen(S[i])+1; }
  int n2 = sequence::scan(L,L,m,utils::addF<int>(),0);
  char* Chars = newA(char,n2);
  char** Strings = newA(char*, m+1);
  { cilk_for(int i=0; i < m; i++) {
      Strings[i] = Chars + L[i];
      strcpy(Strings[i],S[i]); 
    }}
  Strings[m] = Chars+n2;
  free(L);
  return stringA(Chars,n2,Strings,m);
}

// For each string of length n it creates n copies of length n-1
// each with a different letter missing.  All strings are null 
// terminated.
stringA expandStrings(stringA SS) {
  int m = SS.m;
  int *L = newA(int,m);
  int *LL = newA(int,m);
  cilk_for(int i=0; i < m; i++) {
    L[i] = SS.Strings[i+1] - SS.Strings[i] - 1;
    LL[i] = L[i]*L[i]; // total length needed including nulls
  }
  int n2 = sequence::scan(LL,LL,m,utils::addF<int>(),0);
  int *O = newA(int,m);
  int n3 = sequence::scan(L,O,m,utils::addF<int>(),0);
  char *Chars = newA(char,n2);
  char **Strings = newA(char*,n3+1);
  cilk_for(int i=0; i < m; i++) {
    char* Sin = SS.Strings[i];
    int oOffset = LL[i];
    // for each char position create a string
    for (int j=0; j < L[i]; j++) { 
      Strings[O[i]+j] = Chars+oOffset;
      for (int k=0; k < L[i]+1; k++)
	if (j!=k) Chars[oOffset++] = Sin[k]; // skip j==k character
    }
  }
  Strings[n3] = Chars+n2;
  return stringA(Chars,n2,Strings,n3);
}

typedef pair<int,int> pint;
struct firstF {int operator() (pair<int,int> a) {return a.first;} };

struct vertex {
  char* str;
  int degree;
  int* neighbors;
  vertex() : degree(0), neighbors(NULL) {}
};

struct EditGraph {
  int n; // number of vertices
  vertex* V; // the vertex array
  StrTable Table; // table to lookup vertex id from string;
  EditGraph(int nn, vertex* VV, StrTable T) : n(nn), V(VV), Table(T) {}
};

// Generates edges from each string in the union of S (original strings)
// and SE (the strings with chars dropped) to any string in the same
// set with one additional character.
// The union is 
EditGraph findEdges(stringA S, stringA SE) {
  StrTable T = makeStrTable(SE.m);
  cilk_for(int i=0;i<S.m;i++) T.insert(S.Strings[i]);
  cilk_for(int i=0;i<SE.m;i++) T.insert(SE.Strings[i]);
  seq<char*> Entries = T.entries();
  cout << "Table In size = " << (S.m + SE.m) << " Table out size = " << Entries.size() << endl;
  T.compactLabels();
  int *L = newA(int,S.m);
  cilk_for(int i=0;i<S.m;i++) {
    L[i] = S.Strings[i+1] - S.Strings[i] - 1;
  }
  int *O = newA(int,S.m);
  int n3 = sequence::scan(L,O,S.m,utils::addF<int>(),0);
  pint *ForwardEdges = newA(pint,n3);
  {
    cilk_for (int i=0; i < S.m ; i++) {
      int toV = T.findLabel(S.Strings[i]);
      for (int j=0; j < L[i]; j++) 
        ForwardEdges[O[i]+j] = pint(T.findLabel(SE.Strings[O[i]+j]),toV);
    }
  }
  seq<seq<pint> > G =radixSort::collect(seq<pint>(ForwardEdges,n3),
				       Entries.size(),firstF());

  vertex* V = newA(vertex,Entries.size());
  { cilk_for (int i=0; i < Entries.size(); i++) 
      V[i].str = Entries[i];}
  { cilk_for (int i=0; i < G.size(); i++) {
      seq<pint> NN = G[i];
      int j = NN[0].first;
      V[j].degree = NN.size();
      V[j].neighbors = newA(int,NN.size());
      for (int i=0; i < NN.size(); i++) 
	V[j].neighbors[i] = NN[i].second;
    }
  }
  return EditGraph(Entries.size(), V, T);
}

void buildGraph(char* inS, int n) {
  int plen = 0;
  stringA S1 = parseString(inS,n);
  S1.printSome(plen);
  int DISINCT = 1000000; // assumed upper bound on number of distinct words
  seq<char*> B = removeDuplicates(seq<char*>(S1.Strings,S1.m),DISINCT);
  stringA S2 = packStrings(B.S,B.size());
  S2.printSome(plen);
  S1.del();
  B.del();
  stringA S3 = expandStrings(S2);
  S3.printSome(plen);
  findEdges(S2,S3);
}

// Pads first and last position with null
seq<char> readFile(const char* fname) {
  ifstream file (fname, ios::in|ios::binary|ios::ate);
  int n;
  char *Str;
  utils::myAssert(file.is_open(),"Unable to open file");
  n = file.tellg();
  Str = newA(char,n+2);
  file.seekg (0, ios::beg);
  file.read (Str+1,n);
  file.close();
  Str[0]=0; Str[n+1]=0;
  cout << "read size = " << n << endl;
  return seq<char>(Str,n+2);
}

int cilk_main(int argc, char *argv[]) {
  int t1, t2, n;
  if (argc > 1) n = std::atoi(argv[1]); else n = 6;
  stringstream oss (stringstream::out);
  oss << "/home/guyb/data/text/wikisamp" << n << ".xml";
  startTime();
  seq<char> Str = readFile((oss.str()).c_str());
  nextTime("read file");
  buildGraph(Str.S,Str.size());
  nextTime("build graph");
  return 0;
}
