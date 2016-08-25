#ifndef A_HASH_INCLUDED
#define A_HASH_INCLUDED

#include "parallel.h"
#include "utils.h"
#include "sequence.h"
#include "gettime.h"
#include <stdio.h>
#include <string.h>

using namespace std;

#define MINIMUM_CAPACITY 		(32) 
#define MAXIMUM_CAPACITY 		(1 << 30) 
#define DEFAULT_LOAD_FACTOR   (3) 
#define numthreads 1

template<class HASH, class intT>
class Table {
 public:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  intT m;
  eType empty;
  HASH hashStruct;
  float load;
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
	if(0 == _freeList[iSegment][iCheckFree]._hash && empty == _freeList[iSegment][iCheckFree]._keyvalue) {
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
    fprintf(stderr, "	resize\n");
    int i, j;

    Entry **assumedTable = _table;
    for (i = 0; i < (_segmentMask+1); ++i) {
      if (assumedTable != _table)
	break;
      _segments[i].Lock();
    }

    if (i == (_segmentMask+1))
      rehash();

    for (j = 0; j < i; ++j)
      _segments[j].Unlock();
  }

  void rehash() {
    fprintf(stderr, "	rehash\n");
    _votesForResize = 0; // reset

    Entry **oldTable = _table;
    int oldCapacity = _tablesize;

    if (oldCapacity >= MAXIMUM_CAPACITY) {
      _threshold = INT_MAX; // avoid re-triggering
      return;
    }

    int newCapacity = oldCapacity << 1;
    Entry **newTable = CreateNewTable(newCapacity);
    int mask = newCapacity - 1;

    for (int i = 0; i < oldCapacity ; i++) {
      // We need to guarantee that any existing reads of old Map can
      //  proceed. So we cannot yet null out each bin.  
      Entry *e = oldTable[i];

      if (e != NULL) {
	int idx = e->_hash & mask;
	Entry *_next = e->_next;

	//  Single node on list
	if (_next == NULL) {
	  newTable[idx] = e;
	  oldTable[i] = NULL;
	}
	else {    
	  // Reuse trailing consecutive sequence of all same bit
	  Entry *lastRun = e;
	  int lastIdx = idx;
	  for (Entry *last = _next; last != NULL; last = last->_next) {
	    int k = last->_hash & mask;
	    if (k != lastIdx) {
	      lastIdx = k;
	      lastRun = last;
	    }
	  }
	  newTable[lastIdx] = lastRun;

	  // Clone all remaining nodes
	  Entry *beforep = NULL;
	  Entry *newnode;
	  for (Entry *p = e; p != lastRun; beforep = p, p = p->_next) {
	    int k = p->_hash & mask;
	    newnode = (Entry*) malloc(sizeof(Entry));
	    newnode->_keyvalue = p->_keyvalue;
	    newnode->_next = newTable[k];
	    newTable[k] = newnode;
	  }
	  if (beforep)
	    beforep->_next = NULL;
	  else
	    oldTable[i] = NULL;
	}
      }
      Entry *head = oldTable[i];
      Entry *_next;
      while (head) {
	_next = head->_next;
	free(head);
	head = _next;
      }
    }

    delete [] oldTable;
    _table = newTable;
    _tablesize = newCapacity;
  }


  // Ctors ...................................................................
 Table(int size, HASH hashF, float _load)
   /* ChainedHashMap(const int	initial_capacity	= 32*1024,	//Use the maximum number of keys you are going to use */
   /* 					const int	concurrency_level	= 16,		//Number of updating threads */
   /* 					float			loadFactor			= 0.75, */
   /* 					bool			isPreAlloc			= false) */
   :	load(0.75),
    m(size),
    empty(hashF.empty()),
    hashStruct(hashF),
    concurrency_level((numthreads == 1) ? 1 : min(size/10,numthreads*1000)), 
    _segmentMask  ( NearestPowerOfTwo(concurrency_level) - 1),
    _segmentShift ( CalcDivideShift(NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1) ),
    _bucketMask	  ( NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1 )
      {
	timer initTime;
	initTime.start();
	_loadFactor = load;

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
  // Ctors ...................................................................
 Table(int size, HASH hashF)
   :	load(0.75),
    m(size),
    empty(hashF.empty()),
    hashStruct(hashF),
    concurrency_level((numthreads == 1) ? 1 : min(size/10,numthreads*1000)), 
    _segmentMask  ( NearestPowerOfTwo(concurrency_level) - 1),
    _segmentShift ( CalcDivideShift(NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1) ),
    _bucketMask	  ( NearestPowerOfTwo(m/(NearestPowerOfTwo(concurrency_level)))-1 )
      {
	timer initTime;
	initTime.start();
	_loadFactor = load;

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

  // Query Operations ........................................................
  inline eType find(const kType& key) {
    const unsigned int hkey( hashStruct.hash(key) ); 

    // Try first without locking...
    const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
    Segment& segment( _segments[iSegment] );
    Entry *first = _table[ (hkey & _bucketMask) + _segments[iSegment]._start_indx];
    Entry *e;

    for (e = first; e != NULL; e = e->_next) {
      if (hkey == e->_hash && (0 == hashStruct.cmp(hashStruct.getKey(e->_keyvalue),key))) 
	return e->_keyvalue;
    }
    return empty;
  }

  // Modification Operations .................................................
  inline bool insert(const eType& keyvalue){
    kType vkey = hashStruct.getKey(keyvalue);
	  
    const unsigned int hkey( hashStruct.hash(vkey));
    const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
    Segment& segment( _segments[iSegment] );
    int segcount;
    int votes;
    int index = (hkey & _bucketMask) + _segments[iSegment]._start_indx;
    Entry *first = _table[index];
    //find first without lock
    for (Entry *e = first; e != NULL; e = e->_next) {
      if (hkey == e->_hash && (0 == hashStruct.cmp(hashStruct.getKey(e->_keyvalue),vkey))) {
	if(!hashStruct.replaceQ(keyvalue,e->_keyvalue)) return false;
	else {

	  eType kv = e->_keyvalue;
	  while(hashStruct.replaceQ(keyvalue,kv) && !utils::CAS(&e->_keyvalue,kv,keyvalue)) kv = e->_keyvalue;
	  
	return true;
	}
      }
    }

    //find again with lock
    segment.Lock();
    first = _table[index];

    for (Entry *e = first; e != NULL; e = e->_next) {
      if (hkey == e->_hash && (0 == hashStruct.cmp(hashStruct.getKey(e->_keyvalue),vkey))) {
	if(hashStruct.replaceQ(keyvalue,e->_keyvalue))
	  e->_keyvalue = keyvalue;
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

  inline bool deleteVal(const kType& key) {
    const unsigned int hkey( hashStruct.hash(key) ); 
    const unsigned int  iSegment  (( hkey >> _segmentShift) & _segmentMask);
    Segment&            segment   ( _segments[iSegment] );
    int index = (hkey & _bucketMask) + _segments[iSegment]._start_indx;
    Entry *first = _table[index];
    Entry *e = first;
    //first find without lock
    for (;;) {
      if (e == NULL) {
	return NULL;
      }
      if (hkey == e->_hash && 
	  (0 == hashStruct.cmp(hashStruct.getKey(e->_keyvalue),key)))
	break;
      e = e->_next;
    }

    //try again with lock
    segment.Lock();

    first = _table[index];
    e = first;

    for (;;) {
      if (e == NULL) {
	segment.Unlock();
	return NULL;
      }
      if (hkey == e->_hash && 
	  (0 == hashStruct.cmp(hashStruct.getKey(e->_keyvalue),key)))
	break;
      e = e->_next;
    }

    // All entries following removed node can stay in list, 
    // but all preceding ones need to be cloned.
    eType rc(e->_keyvalue);
    Entry* newFirst = e->_next;
    Entry* needDel[16];
    int	   numNeedDel(1);
    needDel[0] = e;
    for (Entry* p = first; p != e; ) { 
      Entry* newEntry		= GetNewEntry(iSegment);
      newEntry->_hash		= p->_hash;
      newEntry->_keyvalue		= p->_keyvalue;
      //newEntry->_value		= p->_value;
      newEntry->_next		= newFirst;
      newFirst = newEntry;
      needDel[numNeedDel++] = p;
      p = p->_next;
    }

    _table[index] = newFirst;
    --(_segments[iSegment]._count);
    //
    // IMPORTANT: here we need to ALIGNED_FREE "e", but there is no memory management
    //
    for (int i(0); i<numNeedDel; ++i) {
      FreeEntry(needDel[i]);
    }
    segment.Unlock();
    return true;
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

  void print() {}

  bool isEmpty()	{
    for (int i = 0; i < (_segmentMask+1); ++i) 
      if (_segments[i]._count != 0)
	return false;
    return true;
  }

};

template <class HASH, class ET, class intT>
_seq<ET> removeDuplicates(_seq<ET> S, intT m, HASH hashF) {
  Table<HASH,intT> T(m,hashF,1.0);
  timer remdupstime;
  remdupstime.start();
  ET* A = S.A;
  {parallel_for(intT i = 0; i < S.n; i++) { T.insert(A[i]);}}
  remdupstime.reportNext("inserts");
  _seq<ET> R = T.entries();
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
