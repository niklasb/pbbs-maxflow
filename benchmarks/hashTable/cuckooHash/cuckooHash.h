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

#include "parallel.h"
#include "utils.h"
#include "sequence.h"
#include "gettime.h"
using namespace std;

#define FIND_NOLOCK 

inline void acquire(volatile int* x) {
  do {
    if(0 == *x &&
       (0 == __sync_lock_test_and_set(x, 0xFF))) {
      return;
    }
  } while(true);
}

inline void release(volatile int* x) {
  return __sync_lock_release (x);
}

//cuckoo hash
template <class HASH, class intT>
class Table {
 public:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  typedef pair<eType,volatile int> eTypeint;

  intT m;
  intT mask;
  eType empty;
  HASH hashStruct;
  eTypeint* TA; //second element for lock
  intT* compactL;
  float load;


  // needs to be in separate routine due to Cilk bugs
  static void clearA(eTypeint* A, intT n, eType v) {
    parallel_for (intT i=0; i < n; i++) A[i] = make_pair(v,0);
  }

 // needs to be in separate routine due to Cilk bugs
  void clear() {
    parallel_for (intT i=0; i < m; i++) TA[i] = make_pair(empty,0);
  }

  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    int operator() (eTypeint a) {return e != a.first;}};

  uintT hashToRange(intT h) {return h & mask;}
  intT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  intT secondIndex(kType v) {
    intT offset = 0;
    intT h = firstIndex(v);
    intT h2;
    while((h2=hashToRange(hashStruct.hash2(v)+offset)) == h) offset++;
    return h2;
  }
  intT incrementIndex(intT h) {return hashToRange(h+1);}
  intT decrementIndex(intT h) {return hashToRange(h-1);}
 

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 Table(intT size, HASH hashF, float _load) :
  load(_load),
    m(1 << utils::log2Up(100+(intT)(_load*(float)size))), 
    mask(m-1),
    empty(hashF.empty()),
    hashStruct(hashF), 
    TA(newA(eTypeint,m)),
    compactL(NULL) 
      { clearA(TA,m,empty);
      }

 Table(intT size, HASH hashF) :
  load(2.0),
    m(1 << utils::log2Up(100+(intT)(2.0*(float)size))), 
    mask(m-1),
    empty(hashF.empty()),
    hashStruct(hashF), 
    TA(newA(eTypeint,m)),
    compactL(NULL) 
      { clearA(TA,m,empty);
      }
  
  // Deletes the allocated arrays
  void del() {
    free(TA); 
    if (compactL != NULL) free(compactL);
  }

  bool insert(eType v) {
    kType vkey = hashStruct.getKey(v);
    intT prevLoc; 
    intT swaps = 0;
    const intT maxSwaps = m;

    intT h = firstIndex(vkey); 
    intT h2 = secondIndex(vkey);
    if(h > h2) swap(h,h2);

    eTypeint c, c2;  
#ifdef FIND_NOLOCK
    c = TA[h]; c2 = TA[h2];
    if((c.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c.first)) == 0)
       || (c2.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c2.first)) == 0)) {
      return 0;
    }
#endif

    //get some locks
    acquire(&TA[h].second);
    acquire(&TA[h2].second);

    c = TA[h]; c2 = TA[h2];
    if((c.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c.first)) == 0)
       || (c2.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c2.first)) == 0)) {
      release(&TA[h2].second); release(&TA[h].second);
      return 0;
    }
    else if(c.first == empty) {
      //utils::CAS(&TA[h].first,empty,v);
      TA[h].first = v; 

      release(&TA[h2].second); release(&TA[h].second);
      //__sync_synchronize();

      return 1;
    } else if(c2.first == empty) {
      //utils::CAS(&TA[h2].first,empty,v);
      TA[h2].first = v; 

      release(&TA[h2].second); release(&TA[h].second);
      //__sync_synchronize();

      return 1;
    } else {
      //utils::CAS(&TA[h2].first,c2.first,v);
      TA[h2].first = v;
      __sync_synchronize();
      release(&TA[h2].second); release(&TA[h].second);

      v = c2.first; vkey = hashStruct.getKey(v); prevLoc = h2;
      

    }
    //first guy's locks are released
    
    intT h3, h4;
    eTypeint c3, c4;

    while(1){ 
      if(swaps > maxSwaps) { cout << "too many swaps\n"; abort(); }
      h = firstIndex(vkey);
      h2 = secondIndex(vkey);
      if(h > h2) swap(h,h2);
#ifdef FIND_NOLOCK
      c = TA[h]; c2 = TA[h2];
      if((c.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c.first)) == 0)
	 || (c2.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c2.first)) == 0)) {
	return 1;
      }
#endif

      acquire(&TA[h].second);
      acquire(&TA[h2].second);

      //got locks

      h3 = h; h4 = h2;
      if(h3 == prevLoc) swap(h3,h4);
      c3 = TA[h3]; c4 = TA[h4];
      if((c3.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c3.first)) == 0)
	 || (c4.first != empty && hashStruct.cmp(vkey,hashStruct.getKey(c4.first)) == 0)) {
	release(&TA[h2].second); release(&TA[h].second);

	return 1;
      }
      else if(c3.first == empty) { 
	
	TA[h3].first = v;

	release(&TA[h2].second); release(&TA[h].second);
	//__sync_synchronize();
 
	return 1;
      }
      else {
	//utils::CAS(&TA[h3].first,c3.first,v);
	TA[h3].first = v;
	__sync_synchronize();

	release(&TA[h2].second); release(&TA[h].second);
     
	v = c3.first; vkey = hashStruct.getKey(v); prevLoc = h3; swaps++;
      }
    }
  }

  bool deleteVal(kType v) {
    intT i = firstIndex(v);
    eType c = TA[i].first;
    if(c != empty && hashStruct.cmp(v,hashStruct.getKey(c)) == 0) { TA[i] = make_pair(empty,0); return 1; } 
    intT i2 = secondIndex(v);
    eType c2 = TA[i2].first;
    if(c2 != empty && hashStruct.cmp(v,hashStruct.getKey(c2)) == 0) { TA[i2] = make_pair(empty,0); return 1; }
    return 0;
  }

  // Returns the value if an equal value is found in the table
  // otherwise returns the "empty" element.
  eType find(kType v) {
    intT h = firstIndex(v);
    eType c = TA[h].first;
    if(c != empty && hashStruct.cmp(v,hashStruct.getKey(c)) == 0) 
      return c;
    intT h2 = secondIndex(v);
    eType c2 = TA[h2].first;
    if(c2 != empty && hashStruct.cmp(v,hashStruct.getKey(c2)) == 0) 
      return c2;
    return empty;
  }

  // returns the number of entries
  intT count() {
    return sequence::mapReduce<intT>(TA,m,utils::addF<intT>(),notEmptyF(empty));
  }

  struct getFirst {
    eTypeint* A;
  getFirst(eTypeint* AA) : A(AA) {}
    eType operator() (intT i) {return A[i].first;}
  };

  // returns all the current entries compacted into a sequence
  _seq<eType> entries() {
    bool *FL = newA(bool,m);
    parallel_for (intT i=0; i < m; i++)
      FL[i] = (TA[i].first != empty);
    _seq<eType> R = sequence::pack((eType*)NULL,FL,(int)0,(int)m,getFirst(TA));
    free(FL);
    return R;
  }

  // prints the current entries along with the index they are stored at
  void print() {
    cout << "vals = ";
    for (intT i=0; i < m; i++) 
      if (TA[i].first != empty)
	cout << i << ":" << TA[i].first << ",";
    cout << endl;
  }
};

template <class HASH, class ET, class intT>
_seq<ET> removeDuplicates(_seq<ET> S, intT m, HASH hashF) {
  Table<HASH,intT> T(m,hashF,1.0);
  ET* A = S.A;
  timer remdupstime;
  remdupstime.start();
  {parallel_for(intT i = 0; i < S.n; i++) { T.insert(A[i]);}}
  _seq<ET> R = T.entries();
  remdupstime.stop();
  remdupstime.reportTotal("remdups time");
  T.del(); 
  return R;
}

template <class HASH, class ET>
_seq<ET> removeDuplicates(_seq<ET> S, HASH hashF) {
  return removeDuplicates(S, S.n, hashF);
}

template <class intT>
struct hashInt {
  typedef intT eType;
  typedef intT kType;
  eType empty() {return -1;}
  kType getKey(eType v) {return v;}
  intT hash(kType v) {return utils::hash(v);}
  intT hash2(kType v) { return utils::hash2(v);}
  int cmp(kType v, kType b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType v, eType b) {return 0;}
};

// works for non-negative integers (uses -1 to mark cell as empty)

static _seq<intT> removeDuplicates(_seq<intT> A) {
  return removeDuplicates(A,hashInt<intT>());
}

//typedef Table<hashInt> IntTable;
//static IntTable makeIntTable(int m) {return IntTable(m,hashInt());}
template <class intT>
  static Table<hashInt<intT>,intT > makeIntTable(intT m, float load) {
  return Table<hashInt<intT>,intT >(m,hashInt<intT>(), load);}

struct hashStr {
  typedef char* eType;
  typedef char* kType;

  eType empty() {return NULL;}
  kType getKey(eType v) { 
    return v;}

  uintT hash(kType s) {
    uintT hash = 0;
    while (*s) hash = *s++ + (hash << 6) + (hash << 16) - hash;
    return hash;
  }

  uintT hash2(kType s){ 
    uintT hash = 0;
    while (*s) hash = *s++ + (hash << 17) + (hash << 4) - hash;
    return hash;
  }

  int cmp(kType s, kType s2) {
    while (*s && *s==*s2) {s++; s2++;};
    return (*s > *s2) ? 1 : ((*s == *s2) ? 0 : -1);
  }

  bool replaceQ(eType s, eType s2) {return 0;}
};

static _seq<char*> removeDuplicates(_seq<char*> S) {
  return removeDuplicates(S,hashStr());}

template <class intT>
  static Table<hashStr,intT> makeStrTable(intT m, float load) {
  return Table<hashStr,intT>(m,hashStr(), load);}

template <class KEYHASH, class DTYPE>
struct hashPair {
  KEYHASH keyHash;
  typedef typename KEYHASH::kType kType;
  typedef pair<kType,DTYPE>* eType;
  eType empty() {return NULL;}

  hashPair(KEYHASH _k) : keyHash(_k) {}

  kType getKey(eType v) { return v->first; }

  uintT hash(kType s) { return keyHash.hash(s);}
  uintT hash2(kType s) { return keyHash.hash2(s);}
  int cmp(kType s, kType s2) { return keyHash.cmp(s, s2);}

  bool replaceQ(eType s, eType s2) {
    return s->second > s2->second;}
};

static _seq<pair<char*,intT>*> removeDuplicates(_seq<pair<char*,intT>*> S) {
  return removeDuplicates(S,hashPair<hashStr,intT>(hashStr()));}

struct hashSimplePair {
  typedef pair<intT,intT> eType;
  typedef intT kType;
  eType empty() {return pair<intT,intT>(-1,0);}
  kType getKey(eType v) { return v.first; }
  uintT hash(intT s) { return utils::hash(s);}
  uintT hash2(intT s) { return utils::hash2(s); }
  int cmp(intT v, intT b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType s, eType s2) {return s.second > s2.second;}
};

static _seq<pair<intT,intT> > removeDuplicates(_seq<pair<intT,intT> > A) {
  return removeDuplicates(A,hashSimplePair());
}


#endif // _A_HASH_INCLUDED
