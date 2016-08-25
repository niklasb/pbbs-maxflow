#include <iostream>
#include "parallel.h"
#include "sequence.h"
#include "gettime.h"

using namespace std;

template <class F>
void contend(int numWriters, int numLocs, F f,  int numRounds, bool falseShare){
  timer tm;
  int* scratch = newA(int,numWriters);
  int* A = newA(int,numWriters);
  int* map = newA(int,numLocs);
  {parallel_for(int i=0;i<numLocs;i++) map[i] = utils::hash2(i) % numWriters; }
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

struct writeMin { inline bool operator() (int* a, int b, int* scratch)  
  {return utils::writeMin(a,b);}};

inline bool _Read(int *a, int b, int* scratch) {
  *scratch = *a;
  return 0;
}

struct Read { inline bool operator() (int* a, int b, int* scratch)  
  {return _Read(a,b,scratch);}};


inline bool fetchAdd2(int *a, int b, int* scratch) {
  utils::writeAdd(a,b);
  return 1;
}

struct fetchAdd { inline bool operator() (int* a, int b, int* scratch)  
  {return fetchAdd2(a,b,scratch);}};


inline bool xAdd2(int *a, int b, int* scratch) {
  utils::xaddi(a,b);
  return 1;
}

struct xAdd { inline bool operator() (int* a, int b, int* scratch)  
  {return xAdd2(a,b,scratch);}};


inline bool CAS(int *a, int b, int* scratch) {  
  int c; bool r=0;
  c = *a;
  r=utils::CAS(a,c,b);
  return r;
}

struct CASOnce { inline bool operator() (int* a, int b, int* scratch)  
  {return CAS(a,b,scratch);}};

struct writeOnce {inline bool operator() (int* a, int b, int* scratch) 
  {
    *a=b; 
    return 1;}};

struct ndReserve {inline bool operator() (int* a, int b, int* scratch) 
  { 
    if (*a == INT_MAX)
      return utils::CAS(a,INT_MAX,b); else return 0;}};

struct ndReserveNoCAS {inline bool operator() (int* a, int b, int* scratch) 
  {
    if (*a == INT_MAX) { *a = b; return 1;}
    else return 0;}};

template <class F>
void doContend(int n, F f, int rounds, bool falseShare)
{
  for(int nl=1;nl<=n;nl*=2)
    contend(n,nl,f,rounds,falseShare);
  contend(n,n,f,rounds,falseShare);
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

  cout<<"----------------CAS-IF-FIRST--------------"<<endl;
  doContend(n,ndReserve(),rounds,false_sharing);

  cout<<"----------------WRITE-IF-FIRST--------------"<<endl;
  doContend(n,ndReserveNoCAS(),rounds,false_sharing);

  cout<<"-----------------FETCH-AND-ADD---------------"<<endl;
  doContend(n,fetchAdd(),rounds,false_sharing);

  cout<<"-----------------X-ADD---------------"<<endl;
  doContend(n,xAdd(),rounds,false_sharing);

  cout<<"-----------------CAS---------------"<<endl;
  doContend(n,CASOnce(),rounds,false_sharing);

  cout<<"----------------WRITE--------------"<<endl;
  doContend(n,writeOnce(),rounds,false_sharing);

  cout<<"----------------READ+WRITE-LOCAL--------------"<<endl;
  doContend(n,Read(),rounds,false_sharing);
}
