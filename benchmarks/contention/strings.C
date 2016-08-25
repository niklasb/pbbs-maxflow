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
void contend(char** vals, int numWriters, int numLocs, F f, int numRounds, bool falseShare){
  timer tm;
  char** scratch = newA(char*,numWriters);
  char** A = newA(char*,numWriters);
  int* map = newA(int,numLocs);

  {parallel_for(int i=0;i<numLocs;i++) {
      map[i] = utils::hash2(i) % numWriters; }}
  
  //int* vals = newA(int,numWriters);
  int* destinations = newA(int,numWriters);
  //{parallel_for(int i=0;i<numWriters;i++)vals[i]=utils::hash(i);}
  {parallel_for(int i=0;i<numWriters;i++) {
    destinations[i] = falseShare ? 
      utils::hash2(utils::hash(i)) % numLocs :
      map[utils::hash2(utils::hash(i))  % numLocs];    
    }}

  for(int r=0;r<numRounds;r++) { 
    //reset A
    {parallel_for(int i=0;i<numWriters;i++)A[i]=NULL;}
    tm.start();
    {parallel_for(int i=0;i<numWriters;i++) {
	f(&A[destinations[i]],vals[i],&scratch[i]);
      }
    }
    tm.stop();
  }
  cout << "(rand val to "<<numLocs<<" random locations) :";
  tm.reportT(tm.total());      

  free(A); free(destinations); free(map);
  free(scratch);
}


struct writeMin { 
  bool cmp(char* s, char* s2) {
    if(!s) return 0;
    else if(s && !s2) return 1;
    while (*s && *s==*s2) {s++; s2++;};
    return (*s >= *s2);
  }

  inline bool operator() (char** a, char* b, char** scratch)  
  {
    char* c; bool r=0;
    do {c = *a;}
    while (cmp(b,c) && !(r=utils::CAS(a,c,b)));
    return r;
  }
};


struct writeOnce {inline bool operator() (char** a, char* b, char** scratch) 
  {    *a=b; 
    // __asm__ __volatile__ ("" ::: "memory");
    return 1;}};

struct ndReserve {inline bool operator() (char** a, char* b, char** scratch) 
  { 
    if (*a == NULL)
      return utils::CAS(a,(char*)NULL,b); else return 0;}};

struct ndReserveNoCAS {inline bool operator() (char** a, char* b, char** scratch) 
  {    if (*a == NULL) { *a = b; return 1;}
    else return 0;}};


template <class F>
void doContend(char** vals,int n, F f, int rounds, bool falseShare)
{
  for(int nl=1;nl<=n;nl*=2){
    contend(vals,n,nl,f,rounds,falseShare);}
  contend(vals,n,n,f,rounds,falseShare);
}

int parallel_main (int argc, char* argv[]){
  commandLine P(argc,argv,"[-f] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  seqData D = readSequenceFromFile(iFile);
  char** vals = (char**) D.A;
  int n = D.n;
  int rounds = P.getOptionIntValue("-r",1);
  bool false_sharing = P.getOption("-f");
  if(false_sharing) cout << "With false sharing\n"; 
  else cout<<"No false sharing\n";

  cout<<"#writers = "<<n<<endl;
  cout<<"#rounds = "<<rounds<<endl;
  
  cout<<"-----------------WRITE WITH MIN---------------"<<endl;
  doContend(vals,n,writeMin(),rounds,false_sharing);

  cout<<"----------------CAS-IF-FIRST--------------"<<endl;
  doContend(vals,n,ndReserve(),rounds,false_sharing);

  cout<<"----------------WRITE-IF-FIRST--------------"<<endl;
  doContend(vals,n,ndReserveNoCAS(),rounds,false_sharing);

  cout<<"----------------WRITE--------------"<<endl;
  doContend(vals,n,writeOnce(),rounds,false_sharing);

  free(vals);
  // cout<<"-----------------WRITE WITH MIN (DECREASING)---------------"<<endl;
  // doContendChunked(n,writeMin(),rounds,false_sharing);

  //  cout<<"-----------------FETCH-AND-ADD---------------"<<endl;
  //  doContend(n,fetchAdd(),rounds,false_sharing);

  //  cout<<"-----------------X-ADD---------------"<<endl;
  //  doContend(n,xAdd(),rounds,false_sharing);

  // cout<<"-----------------CAS---------------"<<endl;
  // doContend(n,CASOnce(),rounds,false_sharing);

  // cout<<"----------------READ+WRITE-LOCAL--------------"<<endl;
  // doContend(n,Read(),rounds,false_sharing);

  // cout<<"----------------READ ONLY-----------------"<<endl;
  // doRead_only(n,rounds,false_sharing);
}
