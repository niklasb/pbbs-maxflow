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
#include <stdio.h>
#include <string.h>

using namespace std;

template <class HASH, class intT>
class Table {
 public:
  typedef typename HASH::eType eType;
  typedef typename HASH::kType kType;
  intT m;
  intT mask;
  eType empty;
  HASH hashStruct;
  intT* compactL;
  float load;
  unsigned int emptyHash;
  unsigned int busyHash;
  int concurrencyLevel;
  int num_buckets;
  inline int first_lsb_bit_indx(uint x) {
    if(0==x) 
      return -1;
    return __builtin_ffs(x)-1;
  }

  struct Bucket {
    unsigned int   volatile	_hopInfo;
    unsigned int	 volatile	_hash;
    eType  _keyvalue;
    /* _tKey  volatile	_key; */
    /* _tData volatile	_data; */
    /* void init() { */
    /*   _hopInfo	= 0U; */
    /*   _hash		= emptyHash;//_tHash::_EMPTY_HASH; */
    /*   _keyvalue = empty; */
    /*   /\* _key		= _tHash::_EMPTY_KEY; *\/ */
    /*   /\* _data		= _tHash::_EMPTY_DATA; *\/ */
    /* } */
  };

  struct Segment {
    unsigned int volatile	_timestamp;
    volatile int		_lock;

    void init() {
      _timestamp = 0;
      _lock = 0;
    }

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
    }

  };

  // Fields ...................................................................
  unsigned int volatile		_segmentShift;
  unsigned int volatile		_segmentMask;
  unsigned int volatile		_bucketMask;
  Segment*	volatile	_segments;
  Bucket* volatile	_table;

  // Constants ................................................................
  static const unsigned int _HOP_RANGE		= 32;
  static const unsigned int	_INSERT_RANGE	= 4*1024;
  static const unsigned int _RESIZE_FACTOR = 2;

  // Small Utilities ..........................................................
  void find_closer_free_backet(const Segment* const start_seg, Bucket** free_backet, unsigned int* free_distance) {
    Bucket* move_backet( *free_backet - (_HOP_RANGE - 1) );
    for (int move_free_dist(_HOP_RANGE - 1); move_free_dist > 0; --move_free_dist) {
      unsigned int start_hop_info(move_backet->_hopInfo);
      int move_new_free_distance(-1);
      unsigned int mask(1);
      for (int i(0); i < move_free_dist; ++i, mask <<= 1) {
	if (mask & start_hop_info) {
	  move_new_free_distance = i;
	  break;
	}
      }
      if (-1 != move_new_free_distance) {
	Segment*	const move_segment(&(_segments[((move_backet - _table) >> _segmentShift) & _segmentMask]));
				
	if(start_seg != move_segment)
	  move_segment->Lock();	

	if (start_hop_info == move_backet->_hopInfo) {
	  Bucket* new_free_backet(move_backet + move_new_free_distance);
	  /* (*free_backet)->_data  = new_free_backet->_data; */
	  /* (*free_backet)->_key   = new_free_backet->_key; */
	  (*free_backet)->_keyvalue = new_free_backet->_keyvalue;
	  (*free_backet)->_hash  = new_free_backet->_hash;

	  ++(move_segment->_timestamp);
	  _GLIBCXX_WRITE_MEM_BARRIER;
	  //_tMemory::write_barrier();

	  move_backet->_hopInfo |= (1U << move_free_dist);
	  move_backet->_hopInfo &= ~(1U << move_new_free_distance);

	  *free_backet = new_free_backet;
	  *free_distance -= move_free_dist;

	  if(start_seg != move_segment)
	    move_segment->Unlock();	
	  return;
	}
	if(start_seg != move_segment)
	  move_segment->Unlock();	
      }
      ++move_backet;
    }
    *free_backet = 0; 
    *free_distance = 0;
  }

	


  struct notEmptyF { 
    eType e; notEmptyF(eType _e) : e(_e) {} 
    int operator() (eType a) {return e != a;}};



  // Size is the maximum number of values the hash table will hold.
  // Overfilling the table could put it into an infinite loop.
 Table(intT size, HASH hashF, float _load) :
  load(_load),
    concurrencyLevel(1),
    m(1 << utils::log2Up(100+(intT)(_load*(float)size))), 
    //mask(m-1),
    empty(hashF.empty()),
    hashStruct(hashF), 
    emptyHash(0),
    busyHash(1),
    compactL(NULL) 
      { 
	_segmentMask  = NearestPowerOfTwo(concurrencyLevel) - 1;
	_segmentShift = CalcDivideShift(NearestPowerOfTwo(concurrencyLevel/(NearestPowerOfTwo(concurrencyLevel)))-1) ;
	mask = m-1;
	//ADJUST INPUT ............................
	const unsigned int adjInitCap = utils::log2Up(100+(intT)(_load*(float)size));
	const unsigned int adjConcurrencyLevel = utils::log2Up(concurrencyLevel);
	num_buckets = adjInitCap + _INSERT_RANGE + 1;
	_bucketMask = adjInitCap - 1;

	//_segmentShift = first_msb_bit_indx(_bucketMask) - first_msb_bit_indx(_SEGMENTS_MASK);

	//ALLOCATE THE SEGMENTS ...................
	/* _segments = (Segment*) memalign( (_segmentMask + 1) * sizeof(Segment),64 ); */
	/* _table = (Bucket*) memalign( num_buckets * sizeof(Bucket),64 ); */

	_segments = (Segment*) memalign( (_segmentMask + 1) * sizeof(Segment),64 );
	_table = (Bucket*) memalign( num_buckets * sizeof(Bucket),64 );

	Segment* curr_seg = _segments;
	for (unsigned int iSeg = 0; iSeg <= _segmentMask; ++iSeg, ++curr_seg) {
	  curr_seg->init();
	}
	Bucket* curr_bucket = _table;
	for (unsigned int iElm=0; iElm < num_buckets; ++iElm, ++curr_bucket) {
	  curr_bucket->_hopInfo = 0U;
	  curr_bucket->_hash = emptyHash;
	  curr_bucket->_keyvalue = empty;
	}

      }

  // Deletes the allocated arrays
  void del() {
    free(_table); free(_segments);
  }

  void clear () {
    Segment* curr_seg = _segments;
    for (unsigned int iSeg = 0; iSeg <= _segmentMask; ++iSeg, ++curr_seg) {
      curr_seg->init();
    }

    Bucket* curr_bucket = _table;
    for (unsigned int iElm=0; iElm < num_buckets; ++iElm, ++curr_bucket) {
      curr_bucket->_hopInfo = 0U;
      curr_bucket->_hash = emptyHash;
      curr_bucket->_keyvalue = empty;
    }    
  }

  // Query Operations .........................................................
  inline eType find(const kType key) {
    //CALCULATE HASH ..........................
    const unsigned int hash( hashStruct.hash(key) );

    //CHECK IF ALREADY CONTAIN ................
    const	Segment&	segment(_segments[(hash >> _segmentShift) & _segmentMask]);
    register const Bucket* const elmAry( &(_table[hash & _bucketMask]) );
    register unsigned int hopInfo( elmAry->_hopInfo );

    if(0U ==hopInfo)
      return empty;
    else if(1U == hopInfo ) {
      if(hash == elmAry->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(elmAry->_keyvalue)))
	return elmAry->_keyvalue;
      else return empty;
    } /*else if(2U == hopInfo) {
	const Bucket* currElm( elmAry);  ++currElm;
	if(hash == currElm->_hash && _tHash::IsEqual(key, currElm->_key))
	return true;
	else return false;
	}*/

    const	unsigned int startTimestamp( segment._timestamp );
    while(0U != hopInfo) {
      register const int i( first_lsb_bit_indx(hopInfo) );
      register const Bucket* currElm( elmAry + i);
      if(hash == currElm->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(currElm->_keyvalue)))
	return currElm->_keyvalue;
      hopInfo &= ~(1U << i);
    } 

    //-----------------------------------------
    if( segment._timestamp == startTimestamp)
      return empty;

    //-----------------------------------------
    register const	Bucket* currBucket( &(_table[hash & _bucketMask]) );
    for(int i(0); i<_HOP_RANGE; ++i, ++currBucket) {
      if(hash == currBucket->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(currBucket->_keyvalue)))
	return currBucket->_keyvalue;
    }
    return empty;
  }

  //modification Operations ...................................................
  inline bool insert(const eType keyvalue){
    //CALCULATE HASH ..........................
    kType key = hashStruct.getKey(keyvalue);
    const unsigned int hash( hashStruct.hash(key) );

    //LOCK KEY HASH ENTERY ....................
    Segment&	segment(_segments[(hash >> _segmentShift) & _segmentMask]);
    segment.Lock();
    Bucket* const startBucket( &(_table[hash & _bucketMask]) );

    //CHECK IF ALREADY CONTAIN ................
    register unsigned int hopInfo( startBucket->_hopInfo );
    while(0 != hopInfo) {
      register const int i( first_lsb_bit_indx(hopInfo) );
      const Bucket* currElm( startBucket + i);
      if(hash == currElm->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(currElm->_keyvalue))) {
	//register const _tData rc(currElm->_data);
	segment.Unlock();
	//return rc;
	return false;
      }
      hopInfo &= ~(1U << i);
    }

    //LOOK FOR FREE BUCKET ....................
    register Bucket* free_bucket( startBucket );
    register unsigned int free_distance(0);
    for(; free_distance < _INSERT_RANGE; ++free_distance, ++free_bucket) {
      /* if( (emptyHash == free_bucket->_hash) &&	(emptyHash == _tMemory::compare_and_set(&(free_bucket->_hash), emptyHash, busyHash)) ) */
      if( (emptyHash == free_bucket->_hash) &&	(utils::CAS((uint*)&(free_bucket->_hash),(uint) emptyHash, (uint)busyHash)) )

	break;
    }
    //PLACE THE NEW KEY .......................
    if (free_distance < _INSERT_RANGE) {
      do {
	if (free_distance < _HOP_RANGE) {
	  /* free_bucket->_data   = data; */
	  /* free_bucket->_key		= key; */
	  free_bucket->_keyvalue = keyvalue;
	  free_bucket->_hash   = hash;
	  startBucket->_hopInfo |= (1U << free_distance);
	  segment.Unlock();
	  return true;
	  //return _tHash::_EMPTY_DATA;
	}
	find_closer_free_backet(&segment, &free_bucket, &free_distance);
      } while (0 != free_bucket);
    }

    //NEED TO RESIZE ..........................
    fprintf(stderr, "ERROR - RESIZE is not implemented \n");
    exit(1);
    return false;//return _tHash::_EMPTY_DATA;
  }

  inline bool deleteVal( const kType key ) {
    //CALCULATE HASH ..........................
    const unsigned int hash( hashStruct.hash(key) );

    //CHECK IF ALREADY CONTAIN ................
    Segment&	segment( _segments[(hash >> _segmentShift) & _segmentMask] );
    segment.Lock();
    Bucket* const	startBucket( &(_table[hash & _bucketMask]) );
    register unsigned int hopInfo( startBucket->_hopInfo );

    if(0U ==hopInfo) {
      segment.Unlock();
      return false;
    } else if(1U == hopInfo) {
      if(hash == startBucket->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(startBucket->_keyvalue))) {
	startBucket->_hopInfo &= ~1U;
	startBucket->_hash = emptyHash;
	startBucket->_keyvalue = empty;//_tHash::_EMPTY_KEY;
	//register const _tData rc(startBucket->_data);
	//startBucket->_data = _tHash::_EMPTY_DATA;
	segment.Unlock();
	return true;
      } else {
	segment.Unlock();
	return false;
      }
    }

    do {
      register const int i( first_lsb_bit_indx(hopInfo) );
      Bucket* currElm( startBucket + i);
      if(hash == currElm->_hash && 0 == hashStruct.cmp(key, hashStruct.getKey(currElm->_keyvalue))) {
	register unsigned int mask(1); mask <<= i;
	startBucket->_hopInfo &= ~(mask);
	currElm->_hash = emptyHash;
	currElm->_keyvalue = empty;//_tHash::_EMPTY_KEY;
	//register const _tData rc(currElm->_data);
	//currElm->_data = _tHash::_EMPTY_DATA;
	segment.Unlock();
	return true;
      }

      hopInfo &= ~(1U << i);
    } while(0 != hopInfo);

    //UNLOCK & RETURN WITH NULL DATA ..........
    segment.Unlock();
    return false;
  }

  //status Operations .........................................................
  unsigned int count()	{
    unsigned int counter = 0;
    const unsigned int num_elm( _bucketMask + _INSERT_RANGE );
    for(unsigned int iElm=0; iElm < num_elm; ++iElm) {
      if( emptyHash != _table[iElm]._hash ) {
	++counter;
      }
    }
    return counter;
  }   


  struct getKeyVal {
    Bucket* A;
  getKeyVal(Bucket* AA) : A(AA) {}
    eType operator() (intT i) {return A[i]._keyvalue;}
  };

  _seq<eType> entries() {
    const unsigned int num_elm( _bucketMask + _INSERT_RANGE );
    bool *FL = newA(bool,num_elm);
    parallel_for (intT i=0; i < num_elm; i++) 
      FL[i] = (_table[i]._hash != emptyHash);
    _seq<eType> R = sequence::pack((eType*)NULL,FL,(int)0,(int)num_elm,getKeyVal(_table));
    free(FL);
    //cout<<R.n<<endl;
    return R;
  }

  // Private Static Utilities .................................................
  static unsigned int NearestPowerOfTwo(const unsigned int value)	{
    unsigned int rc( 1 );
    while (rc < value) {
      rc <<= 1;
    }
    return rc;
  }

  static unsigned int CalcDivideShift(const unsigned int _value) {
    unsigned int numShift( 0 );
    unsigned int curr( 1 );
    while (curr < _value) {
      curr <<= 1;
      ++numShift;
    }
    return numShift;
  }

  void print() {
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
  typedef pair<intT,intT> eType;
  typedef intT kType;
  eType empty() {return pair<intT,intT>(-1,0);}
  kType getKey(eType v) { return v.first; }
  uintT hash(intT s) { return utils::hash(s);}
  int cmp(intT v, intT b) {return (v > b) ? 1 : ((v == b) ? 0 : -1);}
  bool replaceQ(eType s, eType s2) {return s.second > s2.second;}
};

static _seq<pair<intT,intT> > removeDuplicates(_seq<pair<intT,intT> > A) {
  return removeDuplicates(A,hashSimplePair());
}


#endif // _A_HASH_INCLUDED
