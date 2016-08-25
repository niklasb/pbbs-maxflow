// This code is part of the paper "A Simple and Practical Linear-Work
// Parallel Algorithm for Connectivity" in Proceedings of the ACM
// Symposium on Parallelism in Algorithms and Architectures (SPAA),
// 2014.  Copyright (c) 2014 Julian Shun, Laxman Dhulipala and Guy
// Blelloch
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

#ifndef A_EHASH_INCLUDED
#define A_EHASH_INCLUDED

#include "parallel.h"
#include "utils.h"
#include "sequence.h"
using namespace std;

template <class HASH, class intT>
class ETable {
 private:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  uintT m;
  uintT mask;
  eType empty;
  HASH hashStruct;
  eType* TA;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eType* A, uintT n, eType v) {
    parallel_for (uintT i=0; i < n; i++) A[i] = v;
  }

  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    int operator() (eType a) {return e != a;}};

  uintT hashToRange(uintT h) {return h & mask;}
  uintT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  uintT incrementIndex(uintT h) {return hashToRange(h+1);}
  uintT decrementIndex(uintT h) {return hashToRange(h-1);}


 public:
  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
  //ETable() : {}
 ETable(uintT size, HASH hashF, eType* A) :
  m(size), //m should be power of 2
    mask(m-1),
    TA(A), //A should be pointer to allocated memory
    empty(hashF.empty()),
    hashStruct(hashF)
      { clearA(TA,m,empty); }


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
	int cmp = hashStruct.cmp(hashStruct.getKey(v),hashStruct.getKey(c));
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
  bool find(kType v) {
    uintT h = firstIndex(v);
    eType c = TA[h]; 
    while (1) {
      if (c == empty) return false;
      int cmp = hashStruct.cmp(v,hashStruct.getKey(c));
      if (cmp >= 0)
	if (cmp == 1) return false;
	else return true;
      h = incrementIndex(h);
      c = TA[h];
    }
  }


  // returns all the current entries compacted into a sequence
  _seq<eType> entries() {
    bool *FL = newA(bool,m);
    parallel_for (uintT i=0; i < m; i++) 
      FL[i] = (TA[i] != empty);//hashStruct.cmp(TA[i],empty) != 0;
    _seq<eType> R = sequence::pack(TA,FL,m);
    free(FL);
    return R;
  }
};

template <class intT>
struct hashInt {
  typedef intT eType;
  typedef intT kType;
  eType empty() {return INT_T_MAX;}
  kType getKey(eType v) {return v;}
  intT hash(kType v) {return utils::hash(v);}
  int cmp(kType v, kType b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType v, eType b) {return 0;}
};

#endif // _A_EHASH_INCLUDED
