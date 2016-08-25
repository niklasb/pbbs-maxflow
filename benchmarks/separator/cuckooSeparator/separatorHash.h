#ifndef S_HASH_INCLUDED
#define S_HASH_INCLUDED
#include "weightedArc.h"
#include "parallel.h"
#include "utils.h"
#include "sequence.h"
using namespace std;

intT arcHash(intPair x){ 
  return utils::hash(x.first) + 3*utils::hash(x.second); 
}

intT arcHash2(intPair x){ 
  return utils::hash2(x.first) + 3*utils::hash2(x.second); 
}

intT arcCmp(intPair x, intPair y) {
  return (x.first > y.first) ? 1 : ((x.first < y.first) ? -1 : 
				    ((x.second > y.second) ? 1 :
				     ((x.second < y.second) ? -1 : 0)));
}

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

class ArcTable {
 public:
  typedef weightedArc eType;
  typedef intPair kType;
  typedef pair<eType,volatile int> eTypeint;

  intT m;
  intT mask;
  weightedArc empty;
  eTypeint* TA;
  intT* compactL;
  float loadFactor;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(eTypeint* A, intT n, weightedArc v) {
    parallel_for (intT i=0; i < n; i++) A[i] = make_pair(v,0);
  }

  struct notEmptyF { 
    weightedArc e; notEmptyF(weightedArc _e) : e(_e) {} 
    int operator() (eTypeint a) {return e.u() != a.first.u();}};

  inline uintT hashToRange(intT h) {return h & mask;}
  inline intT firstIndex(intPair v) {return hashToRange(arcHash(v));}
  inline intT secondIndex(intPair v) {
    intT offset = 0;
    intT h = firstIndex(v);
    intT h2;
    while((h2=hashToRange(arcHash2(v)+offset)) == h) offset++;
    return h2;
  }

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 ArcTable(intT size, float _loadFactor) :
  loadFactor(_loadFactor),
  m(1 << utils::log2Up(100+(intT)(_loadFactor*size))), 
    mask(m-1),
    TA(newA(eTypeint,m)),
    compactL(NULL) 
      { empty=weightedArc(-1,-1,0);clearA(TA,m,empty); }

  // Deletes the allocated arrays
  void del() {
    free(TA); 
    if (compactL != NULL) free(compactL);
  }

  void makeSmaller (intT size) {
    intT newSize = 1<<utils::log2Up(100+(intT)(loadFactor*size));
    if (m < newSize) { cout<<"new size is not smaller\n"; abort(); }
    m = newSize;
    mask = m-1;
    clearA(TA,m,empty);
  }


  bool insert(eType v) {
    kType vkey = v.arc;
    intT prevLoc; 
    intT swaps = 0;
    const intT maxSwaps = m;

    intT h = firstIndex(vkey); 
    intT h2 = secondIndex(vkey);
    if(h > h2) swap(h,h2);

    eTypeint c, c2;  

    //get some locks
    acquire(&TA[h].second);
    acquire(&TA[h2].second);

    c = TA[h]; c2 = TA[h2];
    
    if(c.first.u() != empty.u() && arcCmp(vkey,c.first.arc) == 0) {
      arcCASWeight(&TA[h].first,v.weight);
      release(&TA[h2].second); release(&TA[h].second);
      return 1;
    }
    else if (c2.first.u() != empty.u() && arcCmp(vkey,c2.first.arc) == 0) {
      arcCASWeight(&TA[h2].first,v.weight);
      release(&TA[h2].second); release(&TA[h].second);
      return 1;
    }
    else if(c.first.u() == empty.u()) {
      //utils::CAS(&TA[h].first,empty,v);
      TA[h].first = v; 

      release(&TA[h2].second); release(&TA[h].second);
      //__sync_synchronize();

      return 1;
    } else if(c2.first.u() == empty.u()) {
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

      v = c2.first; vkey = v.arc; prevLoc = h2;
      

    }
    //first guy's locks are released
    
    intT h3, h4;
    eTypeint c3, c4;

    while(1){ 
      if(swaps > maxSwaps) { cout << "too many swaps\n"; abort(); }
      h = firstIndex(vkey);
      h2 = secondIndex(vkey);
      if(h > h2) swap(h,h2);

      acquire(&TA[h].second);
      acquire(&TA[h2].second);

      //got locks

      h3 = h; h4 = h2;
      if(h3 == prevLoc) swap(h3,h4);
      c3 = TA[h3]; c4 = TA[h4];
      if(c3.first.u() != empty.u() && arcCmp(vkey,c3.first.arc) == 0) {
	arcCASWeight(&TA[h3].first,v.weight);
	release(&TA[h2].second); release(&TA[h].second);
	return 1;
      }
      else if(c4.first.u() != empty.u() && arcCmp(vkey,c4.first.arc) == 0) {
	arcCASWeight(&TA[h4].first,v.weight);
	release(&TA[h2].second); release(&TA[h].second);
	return 1;
      }
      else if(c3.first.u() == empty.u()) { 
	
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
     
	v = c3.first; vkey = v.arc; prevLoc = h3; swaps++;
      }
    }
  }

  template <class F>
  void map(F f){ 
    parallel_for(intT i=0;i<m;i++)
      if(TA[i].first.u() != empty.u()) f(TA[i].first);
  }

  template <class F>
  void mapIndex(F f){ 
    parallel_for(intT i=0;i<m;i++)
      if(TA[i].first.u() != empty.u()) f(TA[i].first,i);
  }

  // returns the number of entries
  intT count() {
    return sequence::mapReduce<intT>(TA,m,utils::addF<intT>(),notEmptyF(empty));
  }

  /* // returns all the current entries compacted into a sequence */
  /* _seq<weightedArc> entries() { */
  /*   bool *FL = newA(bool,m); */
  /*   parallel_for (intT i=0; i < m; i++)  */
  /*     FL[i] = (TA[i].u() != empty.u()); */
  /*   _seq<weightedArc> R = sequence::pack(TA,FL,m); */
  /*   free(FL); */
  /*   return R; */
  /* } */

  // prints the current entries along with the index they are stored at
  void print() {
    cout << "vals = ";
    for (intT i=0; i < m; i++)
      if (TA[i].first.u() != empty.u())
  	{ cout << i << ": "; TA[i].first.print(); cout << endl;}
  }
};

#endif
