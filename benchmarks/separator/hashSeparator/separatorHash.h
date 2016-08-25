#ifndef S_HASH_INCLUDED
#define S_HASH_INCLUDED
#include "weightedArc.h"
#include "parallel.h"
#include "utils.h"
#include "sequence.h"
using namespace std;

intT arcHash(intPair x){ 
  return utils::hash(x.first) ^ utils::hash(x.second); 
}

intT arcCmp(intPair x, intPair y) {
  return (x.first > y.first) ? 1 : ((x.first < y.first) ? -1 : 
				    ((x.second > y.second) ? 1 :
				     ((x.second < y.second) ? -1 : 0)));
}

class ArcTable {
 public:
  intT m;
  intT mask;
  weightedArc* empty;
  weightedArc** TA;
  intT* compactL;

  // needs to be in separate routine due to Cilk bugs
  static void clearA(weightedArc** A, intT n, weightedArc* v) {
    parallel_for (intT i=0; i < n; i++) A[i] = v;
  }

  struct notEmptyF { 
    weightedArc* e; notEmptyF(weightedArc* _e) : e(_e) {} 
    int operator() (weightedArc* a) {return e != a;}};

  uintT hashToRange(intT h) {return h & mask;}
  intT firstIndex(intPair v) {return hashToRange(arcHash(v));}
  intT incrementIndex(intT h) {return hashToRange(h+1);}
  intT decrementIndex(intT h) {return hashToRange(h-1);}
  bool lessIndex(intT a, intT b) {return 2 * hashToRange(a - b) > m;}


  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 ArcTable(intT size) :
    m(1 << utils::log2Up(100+2*size)), 
    mask(m-1),
    TA(newA(weightedArc*,m)),
    compactL(NULL) 
      { empty=NULL;clearA(TA,m,empty); }

  // Deletes the allocated arrays
  void del() {
    free(TA); 
    if (compactL != NULL) free(compactL);
  }

  void makeSmaller (intT size) {
    intT newSize = 1<<utils::log2Up(100+2*size);
    if (m < newSize) { cout<<"new size is not smaller\n"; abort(); }
    m = newSize;
    mask = m-1;
    clearA(TA,m,empty);
  }

  // prioritized linear probing
  //   a new key will bump an existing key up if it has a higher priority
  //   an equal key will replace an old key if replaceQ(new,old) is true
  // returns 0 if not inserted (i.e. equal and replaceQ false) and 1 otherwise
  bool insert(weightedArc* v) {
    intPair vkey = v->arc;
    intT h = firstIndex(vkey); 
    //h = 0;
    /* cout<<"m="<<m<<endl; */
    /* cout<<"h="<<h<<endl; */
    while (1) {
      weightedArc* c;
      int cmp;
      bool swapped = 0;
      c = TA[h];

      cmp = (c == NULL) ? 1 : arcCmp(vkey,c->arc);
      
      // while v is higher priority than entry at TA[h] try to swap it in
      while (cmp == 1 && !(swapped=utils::CAS(&TA[h],c,v))) {
	c = TA[h];
	cmp = arcCmp(vkey,c->arc);
      }

      // if swap succeeded either we are done (if swapped with empty)
      // or we have a new lower priority value we have to insert
      if (swapped) {
	if (c == NULL) return 1; // done
	else { v = c; vkey = c->arc;} // new value to insert

      } else {
	// if swap did not succeed then priority of TA[h] >= priority of v
	if(cmp == 0) {
	  utils::writeAdd(&(c->weight),v->weight);
	  return 1;
	}

      } 

      // move to next bucket
      h = incrementIndex(h);
    }
    //abort();
    return 0; // should never get here
  }

  /* typedef std::function<void (weightedArc*)> testfn; */
  template <class F>
  void map(F f){ 
    parallel_for(intT i=0;i<m;i++)
      if(TA[i] != NULL) f(TA[i]);
  }

  // returns the number of entries
  intT count() {
    return sequence::mapReduce<intT>(TA,m,utils::addF<intT>(),notEmptyF(empty));
  }

  // returns all the current entries compacted into a sequence
  _seq<weightedArc*> entries() {
    bool *FL = newA(bool,m);
    parallel_for (intT i=0; i < m; i++) 
      FL[i] = (TA[i] != NULL);
    _seq<weightedArc*> R = sequence::pack(TA,FL,m);
    free(FL);
    return R;
  }

  // prints the current entries along with the index they are stored at
  void print() {
    cout << "vals = ";
    for (intT i=0; i < m; i++) 
      if (TA[i] != NULL)
	cout << i << ": (" << TA[i]->arc.first << "," << TA[i]->arc.second<<","<<TA[i]->weight<<"), ";
    cout << endl;
  }
};

#endif
