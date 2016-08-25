// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and the PBBS team
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

#ifndef A_HASH_INCLUDED
#define A_HASH_INCLUDED

#include "cilk.h"
#include "utils.h"
#include "sequence.h"
#include "seq.h"
using namespace std;

// A "history independent" hash table that supports insertion, and searching 
// It is described in the paper
//   Guy E. Blelloch, Daniel Golovin
//   Strongly History-Independent Hashing with Applications
//   FOCS 2007: 272-282
// At any quiescent point (when no operations are actively updating the
//   structure) the state will depend only on the keys it contains and not
//   on the history of the insertion order.
// Insertions can happen in parallel, but they cannot overlap with searches
// Searches can happen in parallel
// Deletions must happen sequentially
template <class HASH>
class Table {
 private:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  intT m;
  intT mask;
  eType empty;
  HASH hashStruct;
  eType* TA;
  intT* compactL;
  bool allocated;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eType* A, intT n, eType v) {
    cilk_for (intT i=0; i < n; i++) A[i] = v;
  }

  struct addF { intT operator() (intT a, intT b) {return a+b;}};
  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    intT operator() (eType a) {return e != a;}};

  intT hashToRange(uintT h) {return h & mask;}
  intT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  intT incrementIndex(intT h) {return hashToRange(h+1);}

 public:
  void setUpTable(intT size, eType* space, intT ssize) {
    m = 1 << utils::logUp(2*size);
    mask = m-1;
    empty = hashStruct.empty();
    TA = space;
    allocated = false;  
    if (space == NULL || sizeof(eType)*m > ssize) {
      TA = newA(eType,m); 
      allocated = true;
    }
    clearA(TA,m,empty);
    compactL = NULL;
  }

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
  Table(intT size, HASH hashF) : hashStruct(hashF) { 
    setUpTable(size, NULL, 0);
  }

  Table(intT size, HASH hashF, eType* space, intT ssize) : hashStruct(hashF) { 
      setUpTable(size, space, ssize);
  }

  // Deletes the allocated arrays
  void del() {
    if (allocated) free(TA); 
    if (compactL != NULL) free(compactL);
  }

  // prioritized linear probing
  //   a new key will bump an existing key up if it has a higher priority
  //   an equal key will replace an old key if replaceQ(new,old) is true
  // returns 0 if not inserted (i.e. equal and replaceQ false) and 1 otherwise
  bool insert(eType v) {
    kType vkey = hashStruct.getKey(v);
    intT h = firstIndex(vkey);
    intT round = 0;
    while (1) {
      eType c;
      intT cmp;
      bool swapped = 0;
      c = TA[h];
      cmp = (c==empty) ? 1 : hashStruct.cmp(vkey,hashStruct.getKey(c));

      // while v is higher priority than entry at TA[h] try to swap it in
      while (cmp == 1 && !(swapped=utils::CAS(&TA[h],c,v))) {
	c = TA[h];
	cmp = hashStruct.cmp(vkey,hashStruct.getKey(c));
      }

      // if succeeded then check if done and if not replace v with c
      if (swapped) {
	if (c==empty) return 1;
	else { v = c; vkey = hashStruct.getKey(v);}

      } else {
        // while equal keys either quit or try to replace
	while (cmp == 0) {
	  // if other equal element need not be replaced then quit
	  if (!hashStruct.replaceQ(v,c)) return 0; 

	  // otherwise try to replace (atomically) and quit if successful
	  else if (utils::CAS(&TA[h],c,v)) return 1;

          // otherwise failed due to concurrent write, try again
	  c = TA[h];
	  cmp = hashStruct.cmp(vkey,hashStruct.getKey(c));
	}
      } 

      // move to next bucket
      h = incrementIndex(h);
    }
    return 0; // should never get here
  }

  // Returns the value if an equal value is found in the table
  // otherwise returns the "empty" element.
  // due to prioritization, can quit early if v is greater than cell
  eType find(kType v) {
    intT h = firstIndex(v);
    eType c = TA[h]; 
    while (1) {
      if (c == empty) return empty;
      intT cmp = hashStruct.cmp(v,hashStruct.getKey(c));
      if (cmp >= 0)
	if (cmp == 1) return empty;
	else return c;
      h = incrementIndex(h);
      c = TA[h];
    }
  }

  // returns the number of entries
  intT count() {
    intT *A = newA(intT,m);
    cilk_for (intT i=0; i < m; i++) A[i] = TA[i] != empty;
    intT r = sequence::reduce(A,m,addF());
    free(A);
    return r;
  }

  // returns all the current entries compacted into a sequence
  seq<eType> entries() {
    eType *R = newA(eType,m);
    intT n = sequence::filter(TA,R,m,notEmptyF(empty));
    return seq<eType>(R,n);
  }

  // prints the current entries along with the index they are stored at
  void print() {
    cout << "vals = ";
    for (intT i=0; i < m; i++) 
      if (TA[i] != empty)
	cout << i << ":" << TA[i] << ",";
    cout << endl;
  }
};

template <class HASH, class ET>
seq<ET> removeDuplicates(seq<ET> S, intT m, HASH hashF) {
  Table<HASH> T(m,hashF);
  ET* A = S.S;
  {cilk_for(intT i = 0; i < S.size(); i++) {
      T.insert(A[i]);}}
  seq<ET> R = T.entries();
  T.del(); 
  return R;
}

template <class HASH, class ET>
seq<ET> removeDuplicates(seq<ET> S, HASH hashF) {
  return removeDuplicates(S, S.size(), hashF);
}

struct hashInt {
  typedef intT eType;
  typedef intT kType;
  eType empty() {return -1;}
  kType getKey(eType v) {return v;}
  uintT hash(kType v) {return utils::hash(v);}
  intT cmp(kType v, kType b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType v, eType b) {return 0;}
};

// works for non-negative integers (uses -1 to mark cell as empty)
static seq<intT> removeDuplicates(seq<intT> A) {
  return removeDuplicates(A,hashInt());
}

typedef Table<hashInt> IntTable;
static IntTable makeIntTable(intT m) {return IntTable(m,hashInt());}

struct hashStr {
  typedef char* eType;
  typedef char* kType;

  eType empty() {return NULL;}
  kType getKey(eType v) {return v;}

  uintT hash(kType s) {
    uintT hash = 0;
    while (*s) hash = *s++ + (hash << 6) + (hash << 16) - hash;
    return hash;
  }

  intT cmp(kType s, kType s2) {
    if (s2==NULL) return 1;
    while (*s && *s==*s2) {s++; s2++;};
    return (*s > *s2) ? 1 : ((*s == *s2) ? 0 : -1);
  }

  bool replaceQ(eType s, eType s2) {return 0;}
};

static seq<char*> removeDuplicates(seq<char*> S) {
  return removeDuplicates(S,hashStr());}

typedef Table<hashStr> StrTable;
static StrTable makeStrTable(intT m) {return StrTable(m,hashStr());}

#endif // _A_HASH_INCLUDED
