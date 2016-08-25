#include <iostream>
#include "parallel.h"
#include "gettime.h"
#include "utils.h"

using namespace std;


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

template <class F>
void contend(int numWriters, int numLocs, F f,  int numRounds, bool falseShare, int x){
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
    tm.start();
    for(int i=0;i<x;i++) utils::writeMin(&A[destinations[i]],vals[i]);
    {parallel_for(int i=0;i<numWriters;i++) 
    	utils::writeMin(&A[destinations[i]],vals[i]);
    }

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
void doContend(int n, F f, int rounds, bool falseShare, int x)
{
  for(int nl=1;nl<=n;nl*=2)
    contend(n,nl,f,rounds,falseShare,x);
  contend(n,n,f,rounds,falseShare,x);
}



int parallel_main (int argc, char* argv[]){
  bool false_sharing = 1;
  int n = 1000;
  if (argc > 1) n = std::atoi(argv[1]);  
  int rounds = 1;
  int x = 0;
  if(argc > 2) x = min(n,std::atoi(argv[2])); //number of iterates to sequentialize
  if (argc > 3) rounds = std::atoi(argv[3]);
  if (argc > 4) { false_sharing = 0; cout << "No false sharing\n"; }
  else cout<<"With false sharing\n";
  cout<<"#writers = "<<n<<endl;
  cout<<"#sequential iterates = "<<x<<endl;
  cout<<"#rounds = "<<rounds<<endl; 
  cout<<"-----------------WRITE WITH MIN---------------"<<endl;
  doContend(n,writeMin(),rounds,false_sharing,x);
}

