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

#include "utils.h"
#include "sequence.h"
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
  long m;
  intT mask;
  eType empty;
  HASH hashStruct;
  eType* TA;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eType* A, long n, eType v) {
    cilk_for (long i=0; i < n; i++) A[i] = v;
  }

  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    uintT operator() (eType a) {return e != a;}};

  uintT hashToRange(uintT h) {return h & mask;}
  uintT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  uintT incrementIndex(uintT h) {return hashToRange(h+1);}
  
 public:
  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
  Table(long size, HASH hashF) :
    m(1 << utils::log2Up(100+2*size)), 
    mask(m-1),
    empty(hashF.empty()),
    hashStruct(hashF), 
    TA(newA(eType,m))
      { clearA(TA,m,empty); }

  // Deletes the allocated arrays
  void del() {
    free(TA); 
  }

  // prioritized linear probing
  //   a new key will bump an existing key up if it has a higher priority
  //   an equal key will replace an old key if replaceQ(new,old) is true
  // returns 0 if not inserted (i.e. equal and replaceQ false) and 1 otherwise
  bool insert(eType v) {
    uintT i = firstIndex(hashStruct.getKey(v));
    while (1) {
      eType c = TA[i];
      if (c == empty) {
	if (utils::CAS(&TA[i],c,v)) return 1;
      } else {
	intT cmp = hashStruct.cmp(hashStruct.getKey(v),hashStruct.getKey(c));
	if (cmp == 0) {
	  if (!hashStruct.replaceQ(v,c)) return 0; 
	  else if (utils::CAS(&TA[i],c,v)) return 1;
	} else if (cmp == -1) 
	  i = incrementIndex(i);
	else if (utils::CAS(&TA[i],c,v)) {
	  v = c;
	  i = incrementIndex(i);
	}
      }
    }
  }

  // Returns the value if an equal value is found in the table
  // otherwise returns the "empty" element.
  // due to prioritization, can quit early if v is greater than cell
  eType find(kType v) {
    uintT h = firstIndex(v);
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
  uintT count() {
    return sequence::mapReduce<uintT>(TA,m,utils::addF<uintT>(),notEmptyF(empty));
  }
};

#endif // _A_HASH_INCLUDED
