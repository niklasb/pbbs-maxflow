#ifndef S_HASH_INCLUDED
#define S_HASH_INCLUDED
#include "weightedArc.h"
#include "parallel.h"
#include "utils.h"
#include "sequence.h"
#include <stdio.h>
#include <string.h>

using namespace std;


#define MINIMUM_CAPACITY 		(32) 
#define MAXIMUM_CAPACITY 		(1 << 30) 
#define DEFAULT_LOAD_FACTOR   (3) 
#define numthreads 80

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
  typedef intPair kType;
  typedef weightedArc eType;
  intT m;
  intT mask;
  weightedArc empty;
  weightedArc* TA;
  intT* compactL;
  float loadFactor;
  int concurrency_level;

	// INNER CLASSES ...........................................................
  struct Entry {
    unsigned int	volatile	_hash;
    //_tKey				volatile	_key;
    Entry*			volatile	_next;
    //	_tData			volatile	_value;
    eType _keyvalue;
  };

  struct Segment {
    unsigned int volatile	_count;
    unsigned int volatile	_start_indx;
    volatile int _lock;

    void Lock() {
      do {
	if(0 == _lock &&
	    (0 == __sync_lock_test_and_set(&_lock, 0xFF))) {
	  return;
	}
      } while(true);
    }
    void Unlock() {
      __sync_lock_release (&_lock);
      //_lock = 0;
      //_lock.unlock();
    }
  };

  // PROPERTIES ..............................................................
  float				_loadFactor;
  int				_threshold;
  int volatile	_votesForResize;
  unsigned int	_freeSegSize;
  int				_bucketMask;

  unsigned int				_segmentShift;
  unsigned int volatile*	_iFree;
  Entry** volatile		   _table;
  unsigned int volatile	_tablesize;

  int				_segmentMask;
  Entry**			_freeList;
  Segment*			_segments;

	// UTILITIES ................................................................

	// Private Static Utilities .................................................
  inline static unsigned int NearestPowerOfTwo(const unsigned int _value) {
    unsigned int rc( 1 );
    while (rc < _value) {
      rc <<= 1;
    }
    return rc;
  }

  inline static unsigned int CalcDivideShift(const unsigned int _value) {
    unsigned int numShift( 0 );
    unsigned int curr( 1 );
    while (curr < _value) {
      curr <<= 1;
      ++numShift;
    }
    return numShift;
  }

  static int bitcount(int w)
  {
    w -= (0xaaaaaaaa & w) >> 1;
    w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    w = (w + (w >> 4)) & 0x0f0f0f0f;
    w += w >> 8;
    w += w >> 16;
    return w & 0xff;
  }

  static int p2capacity(int initialCapacity)
  {
    int cap = initialCapacity;

    // Compute the appropriate capacity
    int result;
    if (cap > MAXIMUM_CAPACITY || cap < 0) {
      result = MAXIMUM_CAPACITY;
    } else {
      result = MINIMUM_CAPACITY;
      while (result < cap)
	result <<= 1;
    }
    return result;
  }

  Entry** CreateNewTable(int capacity) {
    _threshold = (int)( (capacity * _loadFactor) / (_segmentMask+1)) + 1;
    //fprintf(stderr, "	CreateNewTable: capacity = %d, _threshold = %d\n", capacity, _threshold);
    Entry **newTable = (Entry **)malloc( capacity * sizeof(Entry*) );
    memset(newTable, 0, capacity * sizeof(Entry *));
    return newTable;
  }

  inline Entry* GetNewEntry(const unsigned int iSegment) {
    if(0 != _freeList) {
      unsigned int numChecked( 0 );
      unsigned int iCheckFree( _iFree[iSegment] );
      unsigned int iFoundFree(0);
      bool isFound(false);
      do {
	if(0 == _freeList[iSegment][iCheckFree]._hash && empty.u() == _freeList[iSegment][iCheckFree]._keyvalue.u()) {
	  iFoundFree = iCheckFree;
	  isFound = true;
	}

	++iCheckFree;
	if(iCheckFree >= _freeSegSize)
	  iCheckFree = 0;
	++numChecked;
      } while( /*numChecked < 20 ||*/ !isFound);

      _iFree[iSegment] = iFoundFree;
      return &(_freeList[iSegment][iFoundFree]);
    }
    else {
      return (Entry*) malloc(sizeof(Entry));
    }
  }

  inline void FreeEntry(Entry* ent) {
    if(0 == _freeList) {
      free(ent);
    }
    else {
      ent->_hash  = 0;
      ent->_keyvalue   = empty;
      ent->_next  = 0;
      //ent->_value = 0;
    }
  }

  void resize() {
    return;
  }


  struct notEmptyF { 
    weightedArc e; notEmptyF(weightedArc _e) : e(_e) {} 
    int operator() (weightedArc a) {return e.u() != a.u();}};

  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 ArcTable(intT size, float _load) :
  loadFactor(0.75),
    m(size),
    
    concurrency_level((numthreads == 1) ? 1 : min(size/10,numthreads*1000)), 
    _segmentMask  ( NearestPowerOfTwo(concurrency_level) - 1),
    _segmentShift ( CalcDivideShift(NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1) ),
    _bucketMask	  ( NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1 )
      {
	timer initTime;
	initTime.start();
	empty=weightedArc(-1,-1,0);
	//loadFactor = _load;

	_segments =  (Segment*) malloc((_segmentMask+1)*sizeof(Segment));
	int curr_seg_start_indx=0;
	for (int i = 0; i <= _segmentMask; ++i) {
	  _segments[i]._lock = 0;//.init();
	  _segments[i]._count = 0;
	  _segments[i]._start_indx = curr_seg_start_indx;
	  curr_seg_start_indx += (_bucketMask+1);
	}
	int cap = NearestPowerOfTwo(m);
	_table = CreateNewTable(cap);
	_tablesize = cap;
	_votesForResize = 0;

	//preallocate
	_freeSegSize = (unsigned int) (((2*cap)/(_segmentMask+1)));
	_freeList	= (Entry**)malloc((_segmentMask+1)*sizeof(Entry*));
	_iFree		= (unsigned int*)malloc((_segmentMask+1)*sizeof(unsigned int));
	for (int iSeg(0); iSeg < (_segmentMask+1); ++iSeg) {
	  _freeList[iSeg] = (Entry*)malloc(_freeSegSize*sizeof(Entry));
	  _iFree[iSeg] = 0;
	  for (unsigned int iElm(0); iElm < _freeSegSize; ++iElm) {
	    _freeList[iSeg][iElm]._hash  = 0;
	    _freeList[iSeg][iElm]._keyvalue   = empty;
	    _freeList[iSeg][iElm]._next  = 0;
	    //_freeList[iSeg][iElm]._value = 0;
	  }
	}
	//initTime.reportTotal("init time:");
	//else _freeList=0;
      }


  void del() {
    timer clearTime;
    clearTime.start();
    clear();
    for (int i = 0; i <= _segmentMask; ++i) {
      _segments[i]._lock = 0;//.init();
    }
    free(_segments);
    free(_table);
    if(0 != _freeList) {
      for (int i(0); i < (_segmentMask+1); ++i) {
	free(_freeList[i]);
	_iFree[i] = 0;
      }
      free(_freeList);
    }
    //clearTime.reportTotal("clear time:");
  }


  // Modification Operations .................................................
  inline bool insert(const eType& keyvalue){
    kType vkey = keyvalue.arc;
	  
    const unsigned int hkey( arcHash(vkey));
    const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
    Segment& segment( _segments[iSegment] );
    segment.Lock();
    
    int segcount;
    int votes;
    int index = (hkey & _bucketMask) + _segments[iSegment]._start_indx;
    Entry *first = _table[index];

    for (Entry *e = first; e != NULL; e = e->_next) {
      if (hkey == e->_hash && (0 == arcCmp(e->_keyvalue.arc,vkey))) {
	arcCASWeight(&e->_keyvalue,keyvalue.weight);
	//eType rc(e->_keyvalue);
	segment.Unlock();
	//return rc;
	return false;
      }
    }

    // Add to front of list
    Entry *newEntry = GetNewEntry(iSegment);
    newEntry->_hash = hkey;
    newEntry->_keyvalue = keyvalue;
    //newEntry->_value = data;
    newEntry->_next = first;
    _table[index] = newEntry;

    if ((segcount = ++_segments[iSegment]._count) < _threshold) {
      segment.Unlock();
      return NULL;
    }

    int bit = (1 << (hkey & _segmentMask));
    votes = _votesForResize;
    if ((votes & bit) == 0)
      votes = _votesForResize |= bit;

    segment.Unlock();

    // Attempt resize if 1/4 segments vote,
    // or if this segment itself reaches the overall _threshold.
    // (The latter check is just a safeguard to avoid pathological cases.)
    if (bitcount(votes) >= ((_segmentMask+1) / 4)  || segcount > (_threshold * (_segmentMask+1))) 
      resize();

    return NULL;
  }


  template <class F>
  void map(F f){ 
    parallel_for(intT i=0;i<_tablesize;i++){
      Entry *first = _table[i];
      Entry *e;
      for (e = first; e != NULL; e = e->_next) {
	if(e->_keyvalue.u() != empty.u()) f(e->_keyvalue);
      }
    }
  }

  template <class F>
  void mapIndex(F f){ 
    parallel_for(intT i=0;i<_tablesize;i++){
      Entry *first = _table[i];
      Entry *e;
      for (e = first; e != NULL; e = e->_next) {
	if(e->_keyvalue.u() != empty.u()) f(e->_keyvalue,i);
      }
    }
  }


  _seq<eType> entries() {
    int* bucketSizes = newA(int,_tablesize);
    {parallel_for(int i=0;i<(_tablesize);++i){
	int bucketcount = 0;
	Entry *first = _table[i];
	Entry *e;
	for (e = first; e != NULL; e = e->_next) {
	  bucketcount++; 
	}
	bucketSizes[i] = bucketcount;
      }
    }

    int totalSize = sequence::plusScan(bucketSizes,bucketSizes,_tablesize);
    eType* R = newA(eType,totalSize);
    {parallel_for(int i=0;i<_tablesize;++i){
	int offset = bucketSizes[i];
	Entry *first = _table[i];
	Entry *e;
	for (e = first; e != NULL; e = e->_next) {
	  R[offset++] = e->_keyvalue;
	}
      }}
    free(bucketSizes);
    return _seq<eType>(R,totalSize);
  }


  // .........................................................................
  void clear() {
    for (unsigned int iElm(0); iElm <_tablesize; ++iElm) {
      Entry* elm = _table[iElm];
      while(NULL != elm) {
	Entry* nextElm = elm->_next;
	FreeEntry(elm);
	elm = nextElm ;
      }
    }

    for (int i(0); i < (_segmentMask+1); ++i) {
      _segments[i]._lock = 0;//init();
      _segments[i]._count = 0;
    }

    memset(_table, 0, _tablesize * sizeof(Entry *));
  }


  unsigned int count() {
    int c = 0;
    for (int i = 0; i < (_segmentMask+1); ++i) {
      c += _segments[i]._count;
      //fprintf(stderr, "	size seg %d, %d\n", i, _segments[i]._count);
    }
    return c;
  }
  // prints the current entries along with the index they are stored at
  void print() {
  }
};

#endif
