#include <iostream>
#include <algorithm>
#include "parallel.h"
#include "sequence.h"
#include "sequenceIO.h"
#include "parseCommandLine.h"
#include "gettime.h"
#include <math.h>
#include "deterministicHash.h"
#include <sys/time.h>

using namespace std;
using namespace benchIO;

template <class F>
void contendChunked(int numWriters, int numLocs, F f, int numRounds, bool falseSharing){
  timer tm;
  int* scratch = newA(int,numWriters);
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
  int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++)
      //vals[i]=utils::hash(i);
      vals[i]=numWriters-i;
    //vals[i]=i;
  }
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseSharing ?
      utils::hash2(utils::hash(i)) % numLocs :
      map[utils::hash2(utils::hash(i)) % numLocs];
    }}

  int p = 80;
  int numIters = numWriters/p;

  for(int r=0;r<numRounds;r++){
    {parallel_for(int i=0;i<numWriters;i++)A[i]=INT_MAX;}
    // //flush cache
    // int* garbage = newA(int, numWriters);
    // {parallel_for(int i=0;i<numWriters;i++) garbage[i] = i;}
    // sequence::plusScan(garbage,garbage,numWriters);
    // free(garbage);
    // //end flush cache
    tm.start();
    // {parallel_for(int i=0;i<p;i++){
    // 	for(int j=0;j<numIters;j++)
    // 	  f(&A[destinations[numIters*i+j]],vals[numIters*i+j],&scratch[numIters*i+j]);
    //   }
    // }
    {parallel_for(int i=0;i<p;i++){
    	for(int j=0;j<numIters;j++)
    	  f(&A[destinations[numIters*i+j]],vals[j],&scratch[numIters*i+j]);
      }
    }

    tm.stop();
  }
  cout << "(rand val to "<<numLocs<<" random locations) :";

  tm.reportT(tm.total());

  free(A); free(vals); free(destinations); free(map);
  free(scratch);

}

template <class F>
void contend(int numWriters, int numLocs, F f,  int numRounds, bool falseShare){
  timer tm;
  int* scratch = newA(int,numWriters);
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
  //{parallel_for(int i=0;i<numLocs;i++) map[i] = i*65 % numWriters; }
  int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseShare ? 
      utils::hash2(utils::hash(i)) % numLocs :
      map[utils::hash2(utils::hash(i))  % numLocs];    
    }}


  for(int r=0;r<numRounds;r++) {
    //reset A
    {parallel_for(int i=0;i<numWriters;i++)A[i]=INT_MAX;}

    //flush cache
    // int cacheSize = 1<<23;
    // int* garbage = newA(int,cacheSize);
    // {parallel_for(int i=0;i<cacheSize;i++) garbage[i] = 1;}
    // sequence::plusScan(garbage,garbage,cacheSize);
    // free(garbage);
    //end flush cache
    tm.start();
    {parallel_for(int i=0;i<numWriters;i++) 
	f(&A[destinations[i]],vals[i],&scratch[i]);
    }
    tm.stop();
  }
  cout << "(rand val to "<<numLocs<<" random locations) :";
  tm.reportT(tm.total());      

  free(A); free(vals); free(destinations); free(map);
  free(scratch);
}

template <class F>
void contendCount(int numWriters, int numLocs, F f,  bool falseShare,
		  bool chunked=0){
  int* A = newA(int,numLocs);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }

  int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  int* Counts = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  // {parallel_for(int i=0;i<numWriters;i++) {
  //   destinations[i] = falseShare ? 
  //     utils::hash2(utils::hash(i)) % numLocs :
  //     map[utils::hash2(utils::hash(i))  % numLocs];    
  //   }}
  
  {parallel_for(int i=0;i<numWriters;i++)destinations[i] = 
      utils::hash(utils::hash(i)) % numLocs;}
  {parallel_for(int i=0;i<numLocs;i++)A[i]=INT_MAX;}
  if(chunked) {
    int p = numWriters/numLocs;
    parallel_for(int i=0;i<p;i++)
      for(int j=0;j<numLocs;j++)
	Counts[i] = f(&A[j],vals[i]);
    
    cout << "("<<p<<" chunks, each chunk writes rand val to location i%"<<numLocs<<" of "<<numLocs<<" locations in same order) :";      
  }
  else {
    // int p = 40;
    // parallel_for(int pr=0;pr<p;pr++)
    //   for(int j=0;j<numWriters/p;j++) {
    // 	int index = min(pr*(numWriters/p)+j,numWriters);
    // 	Counts[index] = f(&A[destinations[index]],vals[index]);
    //   }
     parallel_for(int i=0;i<numWriters;i++) Counts[i] = f(&A[destinations[i]],vals[i]);
    // if(randDest) cout << "(rand val to "<<numLocs<<" random locations) :";
    // else
 cout << "(rand val to location i%"<<numLocs<<" of "<<numLocs<<" locations) :";
  }
  //for(int i=0;i<numWriters;i++)cout<<Counts[i]<<" ";cout<<endl;
  int maxAttempts = sequence::reduce(Counts,numWriters,utils::maxF<int>());
  int totalAttempts = sequence::plusReduce(Counts,numWriters);
  free(A); free(vals); free(destinations); free(Counts); free(map);
  cout << "max attempts = " << maxAttempts;
  cout << " total attempts = " << totalAttempts << endl;
}


template <class F>
void decreasingSeq(int numWriters, int numLocs, F f, int numRounds){
  int* scratch = newA(int,numWriters);
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
  int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  //{parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  {parallel_for(int i=0;i<numWriters;i++)vals[i]=40-i;}
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = map[(utils::hash2(utils::hash(i))) % numLocs];
    }}

  {parallel_for(int i=0;i<numLocs;i++)A[i]=INT_MAX;}
  int p = numWriters/numLocs;
  for(int k=0;k<=40;k++){
    timer tm;
    tm.start();
    for(int r=0;r<numRounds;r++){
      if(k==0){ //warm up
	{parallel_for(int i=0;i<p;i++)
	    for(int j=0;j<1;j++)
	      f(&A[destinations[j]],vals[i],&scratch[i]);
	}
      } else {
	{parallel_for(int i=0;i<1;i++)
	    for(int j=0;j<1;j++)
	      for(int l=0;l<k;l++)
		utils::writeMin(&A[destinations[j]],vals[l]);
	  //f(&A[destinations[j]],vals[l],&scratch[i]);
	}
      }
    }
    if(k > 0){ cout << "("<<p<<" chunks  of "<<numLocs<<" locations in same order, decreasing sequence length = "<<k<<") :";
      tm.reportT(tm.total()); }
  }
  free(A); free(vals); free(destinations); free(map);
  free(scratch);
}

struct getLoc {
  int* A, *destinations;
  getLoc(int* _A, int* _destinations) : 
    A(_A), destinations(_destinations) {}
  int operator() (int i) { return A[destinations[i]]; }
};

void read_only(int numWriters, int numLocs, int numRounds, bool falseSharing){
  timer tm;

  int* A = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }

  //{parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseSharing ?
      utils::hash2(utils::hash(i)) % numLocs :
      map[ utils::hash2(utils::hash(i))  % numLocs];
    }}

  for(int r=0;r<numRounds;r++) {
    //reset A
    {parallel_for(int i=0;i<numLocs;i++)A[i]=i;}

    //flush cache
    // int cacheSize = 1<<23;
    // int* garbage = newA(int,cacheSize);
    // {parallel_for(int i=0;i<cacheSize;i++) garbage[i] = 1;}
    // sequence::plusScan(garbage,garbage,cacheSize);
    // free(garbage);
    //end flush cache
    tm.start();
    parallel_for(int i=0;i<numWriters/numLocs;i++)
      sequence::reduce<int>(0,numLocs,utils::minF<int>(),
			    getLoc(A,destinations));
    tm.stop();
  }
  cout << "(rand val to "<<numLocs<<" random locations) :";
  tm.reportT(tm.total());      

  free(A);

}


struct writeMin { inline bool operator() (int* a, int b, int* scratch)  
  {
  
    return utils::writeMin(a,b);}};

inline bool _Read(int *a, int b, int* scratch) {
  *scratch = *a;
  return 0;
}

struct Read { inline bool operator() (int* a, int b, int* scratch)  
  {return _Read(a,b,scratch);}};


inline bool fetchAdd2(int *a, int b, int* scratch) {
  
  // int c; bool r=0;
  // do c = *a; 
  // while (!(r=utils::CAS(a,c,b)));

  utils::writeAdd(a,b);
  return 1;
}

struct fetchAdd { inline bool operator() (int* a, int b, int* scratch)  
  {return fetchAdd2(a,b,scratch);}};


inline bool xAdd2(int *a, int b, int* scratch) {
  
  // int c; bool r=0;
  // do c = *a; 
  // while (!(r=utils::CAS(a,c,b)));
  //int c = *a;
  utils::xaddi(a,b);
  return 1;
}

struct xAdd { inline bool operator() (int* a, int b, int* scratch)  
  {return xAdd2(a,b,scratch);}};


inline bool CAS(int *a, int b, int* scratch) {
  
  int c; bool r=0;
  c = *a;
  //*scratch = *a; //for local read
  r=utils::CAS(a,c,b);
  return r;
}

struct CASOnce { inline bool operator() (int* a, int b, int* scratch)  
  {return CAS(a,b,scratch);}};

struct writeOnce {inline bool operator() (int* a, int b, int* scratch) 
  {
    *a=b; 
    // __asm__ __volatile__ ("" ::: "memory");
    return 1;}};

struct ndReserve {inline bool operator() (int* a, int b, int* scratch) 
  { 
    if (*a == INT_MAX)
      return utils::CAS(a,INT_MAX,b); else return 0;}};

struct ndReserveNoCAS {inline bool operator() (int* a, int b, int* scratch) 
  {
    if (*a == INT_MAX) { *a = b; return 1;}
    else return 0;}};

inline int writeMinCounter(int *a, int b) {
  int c; int r=0;
  do { c = *a;
    if(c > b) r++; else break;
  }
  while (!utils::CAS(a,c,b));
  return r;
}

struct writeMinCount { inline int operator() (int* a, int b) 
  {return writeMinCounter(a,b);}};

inline int writeMinCounter2(int *a, int b) {
  int c; int r=0;
  c = *a;
  if(c > b) utils::CAS(a,c,b);
  do { c = *a;
    if(c > b) r++; else break;
  }
  while (!utils::CAS(a,c,b));
  return r;
}

struct writeMinCount2 { inline int operator() (int* a, int b) 
  {return writeMinCounter2(a,b);}};

inline int fetchAddCounter(int *a, int b) {
  int c; int r=0;
  do { utils::CAS(a,0,0); c = *a; r++; } 
  while (!utils::CAS(a,c,b));
  return r-1;
}

struct fetchAddCount 
{inline int operator() (int* a, int b) { return fetchAddCounter(a,b); }};

template <class F>
void doContend(int n, F f, int rounds, bool falseShare)
{
  for(int nl=1;nl<=n;nl*=2)
    contend(n,nl,f,rounds,falseShare);
  contend(n,n,f,rounds,falseShare);

  //contendChunked(n,n/40,f,rounds,falseShare);
}

template <class F>
void doContendChunked(int n, F f, int rounds, bool falseShare)
{
  for(int nl=1;nl<=n;nl*=2)
    contendChunked(n,nl,f,rounds,falseShare);
  contendChunked(n,n,f,rounds,falseShare);
}

template <class F>
void doContendCount(int n, F f, bool falseShare){
  contendCount(n,1,f,falseShare);
  // for(int nl=1;nl<=n;nl*=2){
  //   contendCount(n,nl,f,falseShare);
  //}
}  

void doRead_only(int n, int rounds,bool falseSharing)
{
  for(int nl=1;nl<=n;nl*=2)
    read_only(n,nl,rounds,falseSharing);
  read_only(n,n,rounds,falseSharing);

}

//dumb CAS
int foo;
inline bool _dumbCAS(int *a, int b) {
  int c; bool r=0;
  do c = *a; 
  while (foo == 1 && !(r=utils::CAS(a,c,b)));
  return r;
}

struct dumbCAS { inline bool operator() (int* a, int b, int* scratch)  
  {
    return _dumbCAS(a,b);}};


//mixed operations test

void mixed(int numWriters, int proportion, int numLocs, int numRounds, bool falseShare){
  timer tm;
  int* scratch = newA(int,numWriters);
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
  //{parallel_for(int i=0;i<numLocs;i++) map[i] = i*65 % numWriters; }
  int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseShare ? 
      utils::hash2(utils::hash(i)) % numLocs :
      map[utils::hash2(utils::hash(i))  % numLocs];    
    }}

  int p = 32;
  long chunk = numWriters/p;
  long timeout = 1;

  for(int r=0;r<numRounds;r++) {
    //reset A
    {parallel_for(int i=0;i<numLocs;i++)A[i]=INT_MAX;}
    long* numOps = newA(long,p);
    parallel_for(int i=0;i<p;i++)numOps[i]=0;

    int shared = 0;
  
    tm.start();
    if(proportion == -1) {
      parallel_for_1(int i=0;i<p;i++) {
	timer myclock;
	double startTime = myclock.getTime();
	double nextTime = startTime;
	while(nextTime-startTime < timeout) {
	  
	  for (int j=0;j<chunk;j++) {
	    int index = min(i*p+j,numWriters);
	    _Read(&A[destinations[index]],vals[index],&scratch[index]);
	  }
	  numOps[i] += chunk;
	  nextTime = myclock.getTime();
	}
	myclock.stop();
      }
    } else {
      parallel_for_1(int i=0;i<p;i++) {

	// int go = 0;
	// cout<<i<<endl;
	// if(i==0) utils::CAS(sharedPtr,0,1); 
	// else {while(0 == go){go = *sharedPtr;}}
	// cout<<i<<endl;
	timer myclock;
	double startTime = myclock.getTime(); 
	double nextTime = startTime;
	while(nextTime-startTime < timeout) {
	  if(i%proportion){
	    for (int j=0;j<chunk;j++) {
	      int index = min(i*p+j,numWriters-1);
	      _Read(&A[destinations[index]],vals[index],&scratch[index]);
	    }
	  }
	  else { 
	    for (int j=0;j<chunk;j++) {
	      int index = min(i*p+j,numWriters-1);
	      CAS(&A[destinations[index]],vals[index],&scratch[index]);
	    }
	  }

	  numOps[i]+=chunk;
	  nextTime = myclock.getTime();
	}

      }
    }

    tm.stop();

    for(int i=0;i<p;i++) {
      if(numOps[i] > 1000000000) {cout<<"foo\n";}
      if(-1 == proportion || i%proportion)
	cout<<"Processor "<<i<<" (READER) numOps = "<<numOps[i]<<endl;
      else cout<<"Processor "<<i<<" (CAS'ER) numOps = "<<numOps[i]<<endl;
    }

    sequence::reduce<int>(scratch,numWriters,utils::minF<int>());
    free(numOps);

  }
  cout << "(rand val to "<<numLocs<<" random locations) :";
  tm.reportT(tm.total());      

  free(A); free(vals); free(destinations); free(map);
  free(scratch);
}


void doMixed(int n, int rounds, bool falseShare)
{

  int p = 32;
  int numLocs = 8;

  for(int ratio=1;ratio<p;ratio*=2) {
    int numCAS=0;
    for(int i=0;i<p;i++)if(i%ratio==0)numCAS++;
    cout<<"numLocs = "<<numLocs<<" num CAS'ers = "<<numCAS<<", num readers = "<<p-numCAS<<endl;
    

    mixed(n,ratio,numLocs,rounds,0);
  }
  cout<<"numLocs = "<<numLocs<<" num CAS'ers = 1, num readers = 31\n";
  
    mixed(n,p,numLocs,rounds,0);

    cout<<"numLocs = "<<numLocs<< " num CAS'ers = 0, num readers = 32\n";
  
    mixed(n,-1,numLocs,rounds,0);

}


int parallel_main (int argc, char* argv[]){
  // int timeout = CLOCKS_PER_SEC * 10;
  // 	clock_t startTime = clock(); 
  // 	while(clock()-startTime < timeout) {
  // 	  cout<<1;
  // 	} cout<<endl;
  // 	return 1;

  bool false_sharing = 1;
  int n = 1000;
  if (argc > 1) n = std::atoi(argv[1]);  
  int rounds = 1;
  if (argc > 2) rounds = std::atoi(argv[2]);
  if (argc > 3) { false_sharing = 0; cout << "No false sharing\n"; }
  else cout<<"With false sharing\n";

  if (argc > 1) foo = 1; else foo = 0;

  cout<<"#writers = "<<n<<endl;
  cout<<"#rounds = "<<rounds<<endl;

  // decreasingSeq(n,n/40,writeMin(),rounds);
  // abort();
  int p = 32;
  long chunk = n/p;
  int secs = 1;

  // cout<<"p = "<<p<<" chunk size = "<<chunk<<" timeout = "<<secs<<" seconds\n";
  // doMixed(n,rounds,false_sharing);
  
  //abort();
 
  cout<<"-----------------WRITE WITH MIN---------------"<<endl;
  doContend(n,writeMin(),rounds,false_sharing);

  cout<<"-----------------WRITE WITH MIN (DECREASING)---------------"<<endl;
  doContendChunked(n,writeMin(),rounds,false_sharing);

  // doContendCount(n,writeMinCount(),false_sharing);
  // doContendCount(n,writeMinCount2(),false_sharing);
  //doContendCount(n,fetchAddCount(),false_sharing);

  // cout<<"-----------------DUMB CAS---------------"<<endl;
  // doContend(n,dumbCAS(),rounds,false_sharing);


  // cout<<"----------------CAS-IF-FIRST--------------"<<endl;
  // doContend(n,ndReserve(),rounds,false_sharing);

  // cout<<"----------------WRITE-IF-FIRST--------------"<<endl;
  // doContend(n,ndReserveNoCAS(),rounds,false_sharing);

  //  cout<<"-----------------FETCH-AND-ADD---------------"<<endl;
  //  doContend(n,fetchAdd(),rounds,false_sharing);

  //  cout<<"-----------------X-ADD---------------"<<endl;
  //  doContend(n,xAdd(),rounds,false_sharing);

  // cout<<"-----------------CAS---------------"<<endl;
  // doContend(n,CASOnce(),rounds,false_sharing);

  // cout<<"----------------WRITE--------------"<<endl;
  //  doContend(n,writeOnce(),rounds,false_sharing);

  // cout<<"----------------READ+WRITE-LOCAL--------------"<<endl;
  // doContend(n,Read(),rounds,false_sharing);

  // cout<<"----------------READ ONLY-----------------"<<endl;
  // doRead_only(n,rounds,false_sharing);
}


// int* expDist(intT s, intT e, intT m) { 
//   int n = e - s;
//   int *A = newA(int,n);
//   int lg = utils::log2Up(n)+1;
//   parallel_for (int i = 0; i < n; i++) {
//     int range = (1 << (utils::hash(2*(i+s))%lg));
//     A[i] = (range + utils::hash(2*(i+s)+1)%range)%m;
//   }
//   return A;
// }
