#ifndef S_HASH_INCLUDED
#define S_HASH_INCLUDED
#include "weightedArc.h"
#include "parallel.h"
#include "utils.h"
#include "sequence.h"
using namespace std;

typedef pair<uint,uint> intPair;

uintT arcHash(intPair x){ 
  return utils::hash(x.first) ^ utils::hash(x.second); 
}

uintT arcCmp(intPair x, intPair y) {
  return (x.first > y.first) ? 1 : ((x.first < y.first) ? -1 : 
				    ((x.second > y.second) ? 1 :
				     ((x.second < y.second) ? -1 : 0)));
}

class ArcTable {
 public:
  uintT m;
  intT mask;
  weightedArc empty;
  weightedArc* TA;
  uintT* compactL;
  float loadFactor;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(weightedArc* A, uintT n, weightedArc v) {
    parallel_for (uintT i=0; i < n; i++) A[i] = v;
  }

  struct notEmptyF { 
    weightedArc e; notEmptyF(weightedArc _e) : e(_e) {} 
    int operator() (weightedArc a) {return e.u() != a.u();}};

  inline uintT hashToRange(uintT h) {return h & mask;}
  inline uintT firstIndex(intPair v) {return hashToRange(arcHash(v));}
  inline uintT incrementIndex(uintT h) {return hashToRange(h+1);}

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 ArcTable(uintT size, float _loadFactor) :
  loadFactor(_loadFactor),
    m((uintT) 1 << utils::log2Up(100+(uintT)(_loadFactor*size))), 
    mask(m-1),
    TA(newA(weightedArc,m)),
    compactL(NULL) 
      { empty=weightedArc(UINT_MAX,UINT_MAX,0);clearA(TA,m,empty); }

  // Deletes the allocated arrays
  void del() {
    free(TA); 
    if (compactL != NULL) free(compactL);
  }

  void makeSmaller (uintT size) {
    uintT newSize = (uintT)1<<utils::log2Up(100+(uintT)(loadFactor*size));
    if (m < newSize) { cout<<"new size is not smaller\n"; abort(); }
    m = newSize;
    mask = m-1;
    clearA(TA,m,empty);
  }

  // nondeterministic insert
  bool insert(weightedArc v) {
    intPair vkey = v.arc;
    uintT h = firstIndex(vkey); 
    while (1) {
      weightedArc c;
      int cmp;
      bool swapped = 0;
      c = TA[h];

      if(c.u() == empty.u() && arcCASEdge(&TA[h],empty.arc,vkey)) {
	//c = TA[h];
	arcCASWeight(&TA[h],v.weight);
	return 1;

	/* while (1) { */
	/*   c = TA[h]; */
	/*   if(arcCASWeight(&TA[h],c.weight,c.weight+v.weight)) return 1; */
	/* } */
      }
      else if (arcCmp(vkey,c.arc) == 0) {
	arcCASWeight(&TA[h],v.weight);
	return 1;
	//fetch and add
	/* while (1) { */
	/*   if(arcCASWeight(&TA[h],c.weight,c.weight+v.weight)) return 1; */
	/*   c = TA[h]; */
	/* } */
      }
    
      // move to next bucket
      h = incrementIndex(h); 
    }
    return 0; // should never get here
  }

  template <class F>
  void map(F f){ 
    parallel_for(uintT i=0;i<m;i++)
      if(TA[i].u() != empty.u()) f(TA[i]);
  }

  template <class F>
  void mapIndex(F f){ 
    parallel_for(uintT i=0;i<m;i++)
      if(TA[i].u() != empty.u()) f(TA[i],i);
  }

  // returns the number of entries
  uintT count() {
    return sequence::mapReduce<uintT>(TA,m,utils::addF<uintT>(),notEmptyF(empty));
  }

  // returns all the current entries compacted into a sequence
  _seq<weightedArc> entries() {
    bool *FL = newA(bool,m);
    parallel_for (uintT i=0; i < m; i++) 
      FL[i] = (TA[i].u() != empty.u());
    _seq<weightedArc> R = sequence::pack(TA,FL,m);
    free(FL);
    return R;
  }

  // prints the current entries along with the index they are stored at
  void print() {
    cout << "vals = ";
    for (uintT i=0; i < m; i++)
      if (TA[i].u() != empty.u())
  	{ cout << i << ": "; TA[i].print(); cout << endl;}
  }
};

#endif
