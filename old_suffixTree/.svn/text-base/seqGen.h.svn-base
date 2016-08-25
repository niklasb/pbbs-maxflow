#include <iostream>
#include "cilk.h"
#include <math.h>
#include "itemGen.h"
#include "stringGen.h"


namespace dataGen {

  struct payload {
    double key;
    double payload[2];
  };

  class payloadCmp : public std::binary_function <payload, payload, bool> {
  public:
    bool operator()(payload const& A, payload const& B) const {
      return A.key<B.key;
    }
  };

  template <class T>
  T* rand(int s, int e) { 
    int n = e - s;
    T *A = newA(T, n);
    for (int i = 0; i < n; i++) { // for some reason cilk_for does not work
      A[i] = hash<T>(i+s);
    }
    return A;
  }

  int* randIntRange(int s, int e, int m) { 
    int n = e - s;
    int *A = newA(int, n);
    for (int i = 0; i < n; i++) // for some reason cilk_for does not work
      A[i] = hash<int>(i+s)%m;
    return A;
  }

  payload* randPayload(int s, int e) { 
    int n = e - s;
    payload *A = newA(payload, n);
    for (int i = 0; i < n; i++) 
      A[i].key = hash<double>(i+s); // breaks with cilk_for
    return A;
  }

  template <class T>
  T* almostSorted(int s, int e) { 
    int n = e - s;
    T *A = newA(T,n);
    for (int i = 0; i < n; i++) A[i] = (T) i;
    for (int i = s; i < s+floor(sqrt((float) e-s)); i++)
      swap(A[utils::hash(2*i)%n],A[utils::hash(2*i+1)%n]);
    return A;
  }

  template <class T>
  T* same(int n, T v) { 
    T *A = newA(T,n);
    for (int i = 0; i < n; i++) A[i] = v;
    return A;
  }

  template <class T>
  T* expDist(int s, int e) { 
    int n = e - s;
    T *A = newA(T,n);
    int lg = utils::log2(n)+1;
    for (int i = 0; i < n; i++) {
      int range = (1 << (utils::hash(2*(i+s))%lg));
      A[i] = hash<T>(range + utils::hash(2*(i+s)+1)%range);
    }
    return A;
  }

  struct Word {
    int s;
    char** A;
    nGramTable T;
    Word(char** _A, nGramTable _T, int _s) : A(_A), T(_T), s(_s) {}
    int operator() (int i) {A[i] = T.word(100*(i+s)); return 1;}
  };

  void _cilk_broken(nGramTable T, char** A, char* AA, long* L, int s, int n) {
    cilk_for (int i = 0; i < n; i++) {
      A[i] = AA + L[i];
      T.word(100*(i+s),A[i],100);
    }
  }

  // allocates all words one after the other
  char** trigramWords(int s, int e) { 
    int n = e - s;
    char **A = new char*[n];
    long *L = new long[n+1];
    nGramTable T = nGramTable();
    cilk_for (int i = 0; i < n; i++) 
      L[i] = T.wordLength(100*(i+s),100);
    long m = sequence::scan(L,L,n,utils::addF<long>(),(long) 0);
    char *AA = new char[m];
    _cilk_broken(T,A,AA,L,s,n);
    free(L);
    A[n] = AA;
    return A;
  }

  void freeWords(char** W, int n) {
    free(W[n]);
    free(W);
  }

};
