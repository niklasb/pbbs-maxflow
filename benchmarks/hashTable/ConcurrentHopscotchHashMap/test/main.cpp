////////////////////////////////////////////////////////////////////////////////
// Multi threaded benchmark example
//
////////////////////////////////////////////////////////////////////////////////
//TERMS OF USAGE
//----------------------------------------------------------------------
//
//	Permission to use, copy, modify and distribute this software and
//	its documentation for any purpose is hereby granted without fee,
//	provided that due acknowledgments to the authors are provided and
//	this permission notice appears in all copies of the software.
//	The software is provided "as is". There is no warranty of any kind.
//
//Authors:
//	Maurice Herlihy
//	Brown University
//      and
//	Nir Shavit
//	Tel-Aviv University
//      and
//	Moran Tzafrir
//	Tel-Aviv University
//
// Date: Dec 2, 2008.
//
////////////////////////////////////////////////////////////////////////////////
// Programmer : Moran Tzafrir (MoranTza@gmail.com)
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//INCLUDE DIRECTIVES
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include "../framework/cpp_framework.h"

#include "Configuration.h"
#include "../data_structures/BitmapHopscotchHashMap.h"
#include "../data_structures/BitmapHopscotchHashMapOpt.h"
#include "../data_structures/ChainedHashMap.h"
#include "../data_structures/HopscotchHashMap.h"

#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include "../../common/parallel.h"
#include "../../common/gettime.h"
#include "../../common/sequenceIO.h"
#include "../../common/parseCommandLine.h"
#include "../../common/sequence.h"

using namespace CMDR;
using namespace std;
using namespace benchIO;

////////////////////////////////////////////////////////////////////////////////
//INNER CLASSES
////////////////////////////////////////////////////////////////////////////////

class HASH_INT {
public:
	//you must define the following fields and properties
	static const unsigned int _EMPTY_HASH;
	static const unsigned int _BUSY_HASH;
	static const int _EMPTY_KEY;
	static const int _EMPTY_DATA;

  inline_ static unsigned int Calc(int key) {
    key = (key+0x7ed55d16) + (key<<12);
    key = (key^0xc761c23c) ^ (key>>19);
    key = (key+0x165667b1) + (key<<5);
    key = (key+0xd3a2646c) ^ (key<<9);
    key = (key+0xfd7046c5) + (key<<3);
    key = (key^0xb55a4f09) ^ (key>>16);

    // key ^= (key << 15) ^ 0xcd7dcd7d;
    // key ^= (key >> 10);
    // key ^= (key <<  3);
    // key ^= (key >>  6);
    // key ^= (key <<  2) + (key << 14);
    // key ^= (key >> 16);
		return key;
	}

	inline_ static bool IsEqual(int left_key, int right_key) {
		return left_key == right_key;
	}

	inline_ static void relocate_key_reference(int volatile& left, const int volatile& right) {
		left = right;
	}

	inline_ static void relocate_data_reference(int volatile& left, const int volatile& right) {
		left = right;
	}
};

const unsigned int HASH_INT::_EMPTY_HASH = 0;
const unsigned int HASH_INT::_BUSY_HASH  = 1;
const int HASH_INT::_EMPTY_KEY  = 0;
const int HASH_INT::_EMPTY_DATA = 0;

class HASH_STRING {
public:
  //you must define the following fields and properties
  static const unsigned int _EMPTY_HASH;
  static const unsigned int _BUSY_HASH;
  static char** _EMPTY_KEY;
  static const int _EMPTY_DATA;

  inline_ static unsigned int Calc(char** s) {
    char* t = *s;
    uint hash = 0;
    while (*t) hash = *t++ + (hash << 6) + (hash << 16) - hash;
    return hash;
  }

  inline_ static bool IsEqual(char** s, char** s2) {
    char* t = *s; char* t2 = *s2;
    while (*t && *t==*t2) {t++; t2++;};
    return (*t == *t2);
  }

  inline_ static void relocate_key_reference(char** volatile& left, const char** volatile& right) {
    //left = right;
  }

  inline_ static void relocate_data_reference(char** volatile& left, const char** volatile& right) {
    //left = right;
  }
};

const unsigned int HASH_STRING::_EMPTY_HASH = 0;
const unsigned int HASH_STRING::_BUSY_HASH  = 1;
char** HASH_STRING::_EMPTY_KEY  = NULL;
const int HASH_STRING::_EMPTY_DATA = 0;


template <class TABLE, class TYPE>
void runIt(TABLE & _ds, TYPE* A, intT nIn, intT nDel, intT nSe) {

  int* B = newA(int,nSe);
 

  // {parallel_for(int i=0;i<1000;i++)_ds.putIfAbsent(i,i);}
  // cout<<_ds.size()<<endl;
  // {parallel_for(int i=0;i<1000;i++)_ds.putIfAbsent(i,i);}
  // cout<<_ds.size()<<endl;
  // {parallel_for(int i=0;i<1000;i++)B[i]=_ds.containsKey(i);}
  // for(int i=0;i<100;i++)cout<<B[i]<<" ";cout<<endl;

  // {parallel_for(int i=0;i<1000;i++)_ds.remove(i*2%1000);}
  // cout<<_ds.size()<<endl;
  // {parallel_for(int i=0;i<1000;i++)B[i]=_ds.containsKey(i);}
  // for(int i=0;i<100;i++)cout<<B[i]<<" ";cout<<endl;

  // abort();
  // {parallel_for(int i=0;i<nIn+nDel+nSe;i++) {
  //     if(i%3 == 0) _ds.putIfAbsent(A[i],A[i]);
  //     else if(i%3 == 1) _ds.remove(A[i]);
  //     else _ds.containsKey(A[i]);
  //   }
  // }
  // nextTime("all");
  nextTime("initialize");

  //  _tm.stop();
  // cout<<_ds.size()<<endl;
  // _tm.start();
  {parallel_for(int i=0;i<nIn;i++)
      _ds.putIfAbsent(A[i].first, A[i].second);}
  nextTime("inserts");
  // _tm.stop();
  // cout<<_ds.size()<<endl;
  // _tm.start();
  _ds.entries();
  nextTime("elements");

  {parallel_for(int i=0;i<nIn;i++)
      _ds.containsKey(A[i].first);}
  nextTime("find inserted");
  // _tm.stop();
  // int a = 0;
  // for(int i=0;i<nIn;i++)if(B[i])a++;
  // cout<<"num found = "<<a<<endl;
  // _tm.start();
  {parallel_for(int i=0;i<nSe;i++)
      _ds.containsKey(A[i+nIn+nDel].first);}
  nextTime("find random");

  {parallel_for(int i=0;i<nIn;i++)
      _ds.remove(A[i].first);}
  nextTime("delete inserted");
  // _tm.stop();
  // cout<<_ds.size()<<endl;
  // _tm.start();
  {parallel_for(int i=0;i<nIn;i++)
      _ds.putIfAbsent(A[i].first, A[i].second);}
  nextTime("inserts");

  {parallel_for(int i=0;i<nDel;i++)
      _ds.remove(A[i+nIn].first);}
  nextTime("delete random");

  
  // _ds.clear();
  // nextTime("clear table");

  // cout<<"all operations below are on "<<nIn<<" elements\n";

  // parallel_for(int i=0;i<nIn;i++)
  //   B[i] = _ds.containsKey(A[i+nIn+nDel]);
  // nextTime("find on empty table");
 
  // parallel_for(int i=0;i<nIn;i++)
  //   _ds.remove(A[i]);
  // nextTime("delete on empty table");

  // parallel_for(int i=0;i<nIn;i++)
  //   _ds.putIfAbsent(A[i],A[i]);
  // nextTime("inserts");

  // parallel_for(int i=0;i<nIn;i++)
  //   B[i] = _ds.containsKey(A[i]);
  // nextTime("find inserted");

  // parallel_for(int i=0;i<nIn;i++)
  //   _ds.remove(A[i]);
  // nextTime("deleted inserted");

  free(B);

}

////////////////////////////////////////////////////////////////////////////////
//MAIN
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
 
  char* iFile = P.getArgument(0);
  
  seqData D = readSequenceFromFile(iFile);
  int dt = D.dt;
  

  int n = D.n;

  intT nIn = n/3;
  intT nDel = n/3;
  intT nSe = n - nIn - nDel;
  cout<<"num inserts = "<<nIn<<" , num deletes = "<<nDel
      <<" , num finds = "<<nSe<<endl;

  int threads = P.getOptionIntValue("-t",1);
  int numSegments = (threads == 1) ? 1 : threads*1000;
  cout<<"num threads = "<<threads<<endl;
  //cout<<"num segments = "<<numSegments<<endl;
  cout<<endl;
  startTime();

  if (dt == intPairT){
    intPair* A = (intPair*) D.A;
    // {

    //   // //bitmap hopscotch
    //   BitmapHopscotchHashMapOpt<int, int, HASH_INT,TTASLock, CMDR::Memory> bm_hopscotchOpt(nIn,min(nIn,numSegments*10));
    //   cout<<"bitmap hopscotch opt\n";
    //   runIt(bm_hopscotchOpt,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }

    // {
    //   // //preallocated chained
    //   ChainedHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory>chained(nIn,min(nIn/10,numSegments),0.75, true);
    //   cout<<"chained hash map (preallocated)\n";
    //   runIt(chained,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }

    // //mtmalloc chained
    //ChainedHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory>_ds(nIn,numSegments,2.0, false);
    // {
    // //hopscotch nd
    //   HopscotchHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory> hopscotch_nd(2*nIn,min(nIn,max(1,numSegments/10)),64,false);
    //   cout<<"hopscotch nd\n";
    //   runIt(hopscotch_nd,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }
    // //hopscotch d
    // {
    //   HopscotchHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory> hopscotch_d(2*nIn,numSegments/10,64,true);
    //   cout<<"hopscotch d\n";
    //   runIt(hopscotch_d,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }  
    {
      // //bitmap hopscotch
      BitmapHopscotchHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory> bm_hopscotch(2*nIn,min(nIn,numSegments*10));
      cout<<"bitmap hopscotch\n";
      runIt(bm_hopscotch,A,nIn,nDel,nSe);
      cout<<endl;
    }
    {
      // //bitmap hopscotch
      BitmapHopscotchHashMapOpt<int, int, HASH_INT, TTASLock, CMDR::Memory> bm_hopscotch(2*nIn,min(nIn,numSegments*10));
      cout<<"bitmap hopscotch opt\n";
      runIt(bm_hopscotch,A,nIn,nDel,nSe);
      cout<<endl;
    }

  }
  else if (dt == stringIntPairT){
    
    stringIntPair* A = (stringIntPair*) D.A;
    pair<char**,int>* AA = new pair<char**,int>[D.n];
    parallel_for (intT i=0; i < D.n; i++) {
      AA[i].first = &(A[i].first);
      AA[i].second = A[i].second;
    }
    //free(A);
    // {
    //   // //bitmap hopscotch
    //   BitmapHopscotchHashMapOpt<char*, int, HASH_STRING,TTASLock, CMDR::Memory> bm_hopscotchOpt(nIn,min(nIn,numSegments*10));
    //   cout<<"bitmap hopscotch opt\n";
    //   runIt(bm_hopscotchOpt,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }

    // {
    //   // //preallocated chained
    //   ChainedHashMap<char*, int, HASH_STRING, TTASLock, CMDR::Memory>chained(nIn,min(nIn/10,numSegments),0.75, true);
    //   cout<<"chained hash map (preallocated)\n";
    //   runIt(chained,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }
    // //mtmalloc chained
    //ChainedHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory>_ds(nIn,numSegments,2.0, false);
    // {
    // //hopscotch nd
    //   HopscotchHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory> hopscotch_nd(2*nIn,min(nIn,max(1,numSegments/10)),64,false);
    //   cout<<"hopscotch nd\n";
    //   runIt(hopscotch_nd,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }
    // //hopscotch d
    // {
    //   HopscotchHashMap<int, int, HASH_INT, TTASLock, CMDR::Memory> hopscotch_d(2*nIn,numSegments/10,64,true);
    //   cout<<"hopscotch d\n";
    //   runIt(hopscotch_d,A,nIn,nDel,nSe);
    //   cout<<endl;
    // }  
    {
      // //bitmap hopscotch
      BitmapHopscotchHashMap<char**, int, HASH_STRING, TTASLock, CMDR::Memory> bm_hopscotch(2*nIn,min(nIn,numSegments*10));
      cout<<"bitmap hopscotch\n";
      runIt(bm_hopscotch,AA,nIn,nDel,nSe);
      cout<<endl;
    }
    {
      // //bitmap hopscotch opt
      BitmapHopscotchHashMapOpt<char**, int, HASH_STRING, TTASLock, CMDR::Memory> bm_hopscotch(2*nIn,min(nIn,numSegments*10));
      cout<<"bitmap hopscotch opt\n";
      runIt(bm_hopscotch,AA,nIn,nDel,nSe);
      cout<<endl;
    }

  }
  //free(A);
  return 0;
}
