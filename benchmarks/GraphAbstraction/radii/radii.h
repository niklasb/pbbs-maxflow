#include "graph.h"
#include "parallel.h"
#include "sequence.h"
#include "gettime.h"
#include <string.h>
using namespace std;

typedef pair<intT,intT> intPair;

struct nonNegFun{bool operator() (intT a) {return (a>=0);}};

template <class ET>
inline void writeWithOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !utils::CAS(a, oldV, newV));
}

//version with n , m and start as inputs
intT* BFS64_hybrid(graph<intT> GA){
  intT n = GA.n;
  intT m = GA.m;
  vertex<intT>* V = GA.V;
  const intT threshold = m/1000;
  
  intT* hooks = newA(intT,n);
  intT* Frontier = newA(intT,m);
  intT* FrontierNext = newA(intT,m);
  long* currVisited = newA(long,n);
  long* nextVisited = newA(long,n);
  intT* radii = newA(intT,n);
  intT* Counts = newA(intT,n);
  intT* flags = newA(intT,n);
  
  intT sampleSize = min<intT>(n,64);
  int round = 0;

  parallel_for(intT i=0;i<n;i++) {
    currVisited[i] = nextVisited[i] = 0;
    radii[i] = -1;
    hooks[i] = INT_T_MAX;
  }
  parallel_for(intT i=0;i<sampleSize;i++){
    radii[i] = 0;
    currVisited[i] = (long) 1<<i;
  }
  intT frontierSize = sampleSize;

  while (1) {
    round++;
    //determine which method to use
    if(frontierSize < threshold){
      //need to compute frontier if switching from spmv to spmspv   
      frontierSize = 0;
      for(intT i=0;i<n;i++){
	if(currVisited[i] != nextVisited[i]){
	  Frontier[frontierSize++] = i;
	}
      }
      //cout<<"round "<<round<<" using spmspv . frontierSize = "<<frontierSize<<endl;      
      parallel_for (intT i=0; i < frontierSize; i++) {
	intT v = Frontier[i];
	Counts[i] = V[v].degree;
	long currVisitedV = currVisited[v];
	long currVisitedNgh;
	for (intT j=0; j < V[v].degree; j++) {
	  intT ngh = V[v].Neighbors[j];
	  currVisitedNgh = currVisited[ngh];
	  //check if neighbor still valid, if so then try to hook
	  if(currVisitedNgh != (currVisitedNgh | currVisitedV))
	    if (hooks[ngh] > v)
	      utils::writeMin(&hooks[ngh],v);
	}
      }
      intT nr = sequence::scan(Counts,Counts,frontierSize,
			       utils::addF<intT>(),(intT)0);
      parallel_for(intT i = 0; i < frontierSize; i++) {
	intT v = Frontier[i];
	intT o = Counts[i];
	long currVisitedV = currVisited[v];
	long currVisitedNgh;
	for (intT j=0; j < V[v].degree; j++) {
	  intT ngh = V[v].Neighbors[j];
	  currVisitedNgh = currVisited[ngh];
	  if(currVisitedNgh == (currVisitedNgh | currVisitedV))
	    FrontierNext[o+j] = -1;
	  else { 
	    writeWithOr(&nextVisited[ngh],currVisitedV);
	    if(radii[ngh] != round)
	      radii[ngh] = round;
	  
	    if(hooks[ngh] == v) {
	      FrontierNext[o+j] = ngh;
	      hooks[ngh] = INT_T_MAX;
	    }
	    else {
	      FrontierNext[o+j] = -1;
	    }
	  }
	}
	writeWithOr(&nextVisited[v],currVisitedV);
      }
      frontierSize = sequence::filter(FrontierNext,Frontier,nr,nonNegFun());
      if(!frontierSize) break;
      swap(nextVisited,currVisited);

    } else {
      /* cout<<"round "<<round<<" using spmv . frontierSize = "<< */
      /* 	frontierSize<<endl; */

      parallel_for(intT i=0;i<n;i++){
	nextVisited[i] = currVisited[i];
	for(intT j=0;j<V[i].degree;j++){
	  intT ngh = V[i].Neighbors[j];
	  nextVisited[i] = nextVisited[i] | currVisited[ngh];
	}
      }

      memset(flags,0,n*sizeof(intT));
      //check if changed
      parallel_for(intT i=0;i<n;i++){	
	if(currVisited[i] != nextVisited[i]){
	  radii[i] = round;
	  flags[i] = 1;
	} 
      }

      frontierSize = sequence::plusScan(flags,flags,n);
      if(!frontierSize) break;
      swap(currVisited,nextVisited);
    }
  }
  free(currVisited); free(nextVisited); free(flags); free(Counts);
  free(Frontier); free(FrontierNext); free(hooks);
  return radii;
}
