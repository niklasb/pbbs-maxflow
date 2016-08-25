#include <iostream>
#include "parallel.h"
#include "gettime.h"
#include "deterministicHash.h"

using namespace std;

typedef pair<intT,intT> intPair;

int* expDist(intT s, intT e, intT m) { 
  int n = e - s;
  int *A = newA(int,n);
  int lg = utils::log2Up(n)+1;
  parallel_for (int i = 0; i < n; i++) {
    int range = (1 << (utils::hash(2*(i+s))%lg));
    A[i] = (range + utils::hash(2*(i+s)+1)%range)%m;
  }
  return A;
}


struct hashTable {
  intPair* TA;
  int n;
  unsigned int (*hashF)(unsigned int);
  hashTable(int _n, unsigned int (*_hashF)(unsigned int)) 
  : n(_n), hashF(_hashF) {
    TA = newA(intPair,n);
    parallel_for(int i=0;i<n;i++) TA[i].first = -1; //empty
  }
  
  void insert(int key, int val){
    int loc = hashF(key) % n;
    TA[loc] = make_pair(key,val);
  }

  int find(int key){
    int loc = hashF(key) % n;
    if(TA[loc].first == key) return TA[loc].second;
    else return INT_MAX;
  }

  void del(){
    free(TA);
  }
};

inline unsigned int myHash(unsigned int a)
{
  return ((unsigned int) 2510212345 * a) %
    (unsigned int) 0xFFFFFFFF;
}


inline bool cachedWriteMin(int* A, int loc, int b, hashTable T) {
  if(T.find(loc) <= b) return 0;
  int c; bool r=0;
  do c = A[loc];
  while (c > b && !(r=utils::CAS(&A[loc],c,b)));
  T.insert(loc,c);
  return r;
}


template <class F, class HASH>
void contend(int numWriters, int numLocs, F f,  int numRounds, bool falseShare, HASH hashF){
  timer tm;
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
  //{parallel_for(int i=0;i<numLocs;i++) map[i] = i*65 % numWriters; }
  int* vals = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  int* destinations = newA(int,numWriters);
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseShare ? 
      utils::hash2(utils::hash(i)) % numLocs :
      map[utils::hash2(utils::hash(i))  % numLocs];    
    }}

  //int* destinations = expDist(0, numWriters, numLocs);

  for(int r=0;r<numRounds;r++) {
    //reset A
    {parallel_for(int i=0;i<numLocs;i++)A[i]=INT_MAX;}

    //flush cache
    // int cacheSize = 1<<23;
    // int* garbage = newA(int,cacheSize);
    // {parallel_for(int i=0;i<cacheSize;i++) garbage[i] = 1;}
    // sequence::plusScan(garbage,garbage,cacheSize);
    // free(garbage);
    //end flush cache
    // int x = min(0,numWriters);
    // for(int i=0;i<x;i++) utils::writeMin(&A[destinations[i]],vals[i]);
    // {parallel_for(int i=0;i<numWriters;i++) 
    // 	utils::writeMin(&A[destinations[i]],vals[i]);
    // }
    int p = 80;
    int csize = numWriters/p;

    hashTable* Tptr = newA(hashTable,p);
    for(int i=0;i<p;i++) Tptr[i] = hashTable(1<<13,myHash);
    tm.start();

    parallel_for(int i=0;i<p;i++) {
      //create cache
      hashTable T = Tptr[i];
      //hashTable T(1<<13,myHash);
      for(int j=csize*i;j<csize*(1+i);j++) {
	//utils::writeMin(&A[destinations[j]],vals[j]);
    	cachedWriteMin(A,destinations[j],vals[j],T);
      }
      T.del();
    }
    // {parallel_for(int i=0;i<numWriters;i++) 
    // 	utils::writeMin(&A[destinations[i]],vals[i]);
    // }

    tm.stop();
  }
  cout << "(rand val to "<<numLocs<<" random locations) :";
  tm.reportT(tm.total());      

  free(A); free(vals); free(destinations); free(map);

}



template <class ET>
inline bool myWriteMin(ET *a, ET b) {
  int range = 10;
  int h = utils::hash(b);
  struct timespec req = {0};
  req.tv_sec = 0;
  req.tv_nsec = 1;

  ET c; bool r=0;
  while(1){
    c=*a;
    if(c <= b) break;
    if(r=utils::CAS(a,c,b)) break;
    req.tv_nsec = (h % range);
    range*=2;
    nanosleep(&req, (struct timespec *)NULL);

  }
  // do c = *a; 
  // while (c > b && !(r=CAS(a,c,b)));
  return r;
}


struct writeMin { inline bool operator() (int* a, int b, int* scratch)  
  {  
    return utils::writeMin(a,b);}};


template <class F>
void doContend(int n, F f, int rounds, bool falseShare)
{
  for(int nl=1;nl<=n;nl*=2)
    contend(n,nl,f,rounds,falseShare,hashInt<int>());
  contend(n,n,f,rounds,falseShare,hashInt<int>());
}



int parallel_main (int argc, char* argv[]){
  bool false_sharing = 1;
  int n = 1000;
  if (argc > 1) n = std::atoi(argv[1]);  
  int rounds = 1;
  if (argc > 2) rounds = std::atoi(argv[2]);
  if (argc > 3) { false_sharing = 0; cout << "No false sharing\n"; }
  else cout<<"With false sharing\n";
  cout<<"#writers = "<<n<<endl;
  cout<<"#rounds = "<<rounds<<endl; 
  cout<<"-----------------WRITE WITH MIN---------------"<<endl;
  doContend(n,writeMin(),rounds,false_sharing);
}

