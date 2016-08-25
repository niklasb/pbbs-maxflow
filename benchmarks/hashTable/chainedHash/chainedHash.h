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
using namespace std;

//linked list nodes
template <class eType>
struct node {
  eType elt;
  node* next;
node(eType _elt, node* _next): elt(_elt), next(_next) {}
};

template <class HASH, class intT>
class Table {
 public:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  intT m;
  eType empty;
  HASH hashStruct;
  node<eType>** TA;
  float load;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eType* A, intT n, eType v) {
    parallel_for (intT i=0; i < n; i++) A[i] = v;
  }

 // needs to be in separate routine due to Cilk bugs
  void clear() {
    parallel_for(intT i=0;i<m;i++){
      if(TA[i]) {
	node<eType>* curr = TA[i];
	while(curr->next != NULL) {
	  node<eType>* tmp = curr->next;
	  free(curr);
	  curr = tmp;
	}
	TA[i] = NULL;
      }
    }
  }

  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    int operator() (eType a) {return e != a;}};

  uintT hashToRange(uintT h) {return h % m;}
  intT firstIndex(kType v) {return hashToRange(hashStruct.hash(v));}
  /* intT incrementIndex(intT h) {return hashToRange(h+1);} */
  /* intT decrementIndex(intT h) {return hashToRange(h-1);} */
  /* bool lessIndex(intT a, intT b) {return 2 * hashToRange(a - b) > m;} */

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 Table(intT size, HASH hashF, float _load) :
  load(_load),
    m(size),
    empty(hashF.empty()),
    hashStruct(hashF), 
    TA(newA(node<eType>*,m))
      { 
	parallel_for (intT i=0; i < m; i++) TA[i] = NULL;
      }

  // Deletes the allocated arrays
  void del() {
    free(TA);
  }

  bool insert(eType v) {
    kType vkey = hashStruct.getKey(v);
    intT h = firstIndex(vkey);
    node<eType>* curr = TA[h];
    while(1) {
      if(TA[h] == NULL) {
	node<eType>* nnode = new node<eType>(v,NULL);
	if(utils::CAS(&TA[h],(node<eType>*)NULL,nnode)) return 1;
	/* TA[h] = nnode; */
	/* return 1; */
      }
      else if(hashStruct.cmp(vkey,hashStruct.getKey(curr->elt)) == 0) {
	return 0;
      }
      else if(hashStruct.cmp(vkey,hashStruct.getKey(curr->elt)) == 1) {
	node<eType>* nnode = new node<eType>(v,curr);
	if(utils::CAS(&TA[h],curr,nnode)) return 1;
	//TA[h] = nnode; return 1;
      }
      else {
	while(1) {
	  node<eType>* next = curr->next;
	  while(next != NULL) {
	    int cmp = hashStruct.cmp(vkey,hashStruct.getKey(next->elt));
	    if(cmp == 0) return 0;
	    else if(cmp == 1) break;
	    curr = next; 
	    next = next->next;
	  }
	  node<eType>* nnode = new node<eType>(v,next);
	  if(utils::CAS(&curr->next,next,nnode)) return 1;
	}
	/* curr->next = nnode; */
	/* return 1; */
      }
    }
  }

  bool deleteVal(kType v) {
    intT h = firstIndex(v);
    node<eType>* curr = TA[h];
    if(curr == NULL) return 0;
    //element is head
    else if(hashStruct.cmp(v,hashStruct.getKey(curr->elt)) == 0) {
      TA[h] = curr->next;
      free(curr);
      return 1;
    } 
    else {
      node<eType>* next = curr->next;
      while(next != NULL) {
	int cmp = hashStruct.cmp(v,hashStruct.getKey(next->elt)); 
	if(cmp == 0) {
	  curr->next = next->next;
	  free(next);
	  return 1;
	} 
	else if(cmp == 1) return 0;
	curr = next;
	next = next->next;
      }
    }
    return 0;
  }

  // Returns the value if an equal value is found in the table
  // otherwise returns the "empty" element.
  // due to prioritization, can quit early if v is greater than cell
  eType find(kType v) {
    intT h = firstIndex(v);
    node<eType>* curr = TA[h];
    while(curr != NULL) {
      int cmp = hashStruct.cmp(v,hashStruct.getKey(curr->elt));
      if(cmp == 0)
	return curr->elt;
      else if (cmp == 1) return empty;
      curr = curr->next;
    }
    return empty;
  }

  // returns the number of entries
  intT count() {
    intT* counts = newA(intT,m);
    {parallel_for(intT i=0;i<m;i++) counts[i] = 0;}
    {parallel_for (intT i=0; i < m; i++)
      if (TA[i] != NULL) {
	node<eType>* curr = TA[i];
	while(curr != NULL){
	  curr = curr->next;
	  counts[i]++;
	}
      }}
    intT c = sequence::plusReduce(counts,m);
    free(counts);
    return c;
  }

  // returns all the current entries compacted into a sequence
  _seq<eType> entries() {
    intT maxLoad = 3*utils::log2Up(m)/utils::log2Up(utils::log2Up(m));
    intT sizeE = m*maxLoad;
    eType* E = newA(eType,sizeE);
    {parallel_for(intT i=0;i<sizeE;i++) E[i] = empty;}
    {parallel_for(intT i=0;i<m;i++) {
      if(TA[i] != NULL) {
	intT start = i*maxLoad;
	intT j = 0;
	node<eType>* curr = TA[i];
	while(curr != NULL) {
	  if(j > maxLoad) { cout << "max load exceeded\n"; abort(); }
	  E[start+j++] = curr->elt;
	  curr = curr->next;
	}
      }
      }}
    _seq<eType> R = sequence::filter(E,sizeE,notEmptyF(empty));
    free(E);
    return R;
  }

  // prints the current entries along with the bucket they are stored at
  void print() {
    for (intT i=0; i < m; i++)
      if (TA[i] != NULL) {
	node<eType>* curr = TA[i];
	cout << i << ": ";
	while(curr != NULL){
	  cout << curr->elt << ", ";
	  curr = curr->next;
	}
	cout << endl;
      }
  }
};

template <class HASH, class ET, class intT>
_seq<ET> removeDuplicates(_seq<ET> S, intT m, HASH hashF) {
  Table<HASH,intT> T(m,hashF,2.0);
  ET* A = S.A;
  {parallel_for(intT i = 0; i < S.n; i++) { T.insert(A[i]);}}
  _seq<ET> R = T.entries();
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
  int cmp(kType s, kType s2) { return keyHash.cmp(s, s2);}

  bool replaceQ(eType s, eType s2) {
    return s->second > s2->second;}
};

static _seq<pair<char*,intT>*> removeDuplicates(_seq<pair<char*,intT>*> S) {
  return removeDuplicates(S,hashPair<hashStr,intT>(hashStr()));}

struct hashSimplePair {
  typedef pair<int,int> eType;
  typedef int kType;
  eType empty() {return pair<int,int>(-1,0);}
  kType getKey(eType v) { return v.first; }
  uintT hash(int s) { return utils::hash(s);}
  int cmp(int v, int b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType s, eType s2) {return s.second > s2.second;}
};

static _seq<pair<int,int> > removeDuplicates(_seq<pair<int,int> > A) {
  return removeDuplicates(A,hashSimplePair());
}


#endif // _A_HASH_INCLUDED
