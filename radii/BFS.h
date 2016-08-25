#include "graph.h"
#include "parallel.h"
#include "sequence.h"
#include "gettime.h"
#include <string.h>
using namespace std;

typedef pair<intT,intT> intPair;

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

intT BFS(intT start, graph& A,
		    intT* Frontier, intT* Visited,
		    intT* FrontierNext, intT* Counts) {
  intT numVertices = A.n;
  intT numEdges = A.m;
  vertex* V = A.V;
  {parallel_for(intT i = 0; i < numVertices; i++) Visited[i] = 0;}

  Frontier[0] = start;
  intT frontierSize = 1;
  Visited[start] = 1;

  //intT totalVisited = 0;
  int round = 0;

  while (frontierSize > 0) {
    round++;
    //totalVisited += frontierSize;

    parallel_for (intT i=0; i < frontierSize; i++) {
      intT id = Frontier[i];
      Counts[i] = V[id].degree;
    }
    intT nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);

    // For each vertex in the frontier try to "hook" unvisited neighbors.
    parallel_for(intT i = 0; i < frontierSize; i++) {
      intT v = Frontier[i];
      intT o = Counts[i];
      for (intT j=0; j < V[v].degree; j++) {
        intT ngh = V[v].Neighbors[j];
	if (Visited[ngh] == 0 && utils::CAS(&Visited[ngh],(intT)0,(intT)1)) {
	  FrontierNext[o+j] = ngh;
	}
	else FrontierNext[o+j] = -1;
      }
      
    }
    // Filter out the empty slots (marked with -1)
    frontierSize = sequence::filter(FrontierNext,Frontier,nr,nonNegF());
  }
  return round-1;
}

struct notX{
  const intT x;
  notX(intT _x) : x(_x) {}
  bool operator() (const intT a) {return (a!=x);
  }
};


template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !utils::CAS(a, oldV, newV));
}



//version with n , m and start as inputs
intT BFS64(graph& A, intT n, intT m, intT start,
	  int* hooks,
	  int* Frontier, long* currVisited, long* nextVisited,
	  int* FrontierNext, intT* Counts,
	   int* lastChangedTime, int K=64) {

  intT work = 0;
  intT numVertices = n;
  intT numEdges = m;
  vertex* V = A.V;

  /* intT* tmp = Counts; */
  /* parallel_for(intT i=0;i<numVertices;i++) */
  /*   if(currVisited[i+start]) {tmp[i] = 1; */
  /*     //if(getRadii) lastChangedTime[i+start] = 0; */
  /*   } */
  /*   else tmp[i] = 0; */
  /* intT frontierSize = sequence::plusScan(tmp,tmp,numVertices); */

  intT frontierSize = 0;
  int maxNum = min<intT>(numVertices,K);
  for(intT i=start;i<start+numVertices;i++){
    if(currVisited[i]) {
      Frontier[frontierSize++] = i; 
      //cout<<i<<" ";
      if(frontierSize >= maxNum) break;
    }
  }
  //cout<<endl;
  //for(int i=0;i<numVertices;i++)cout<<tmp[i]<<" ";cout<<endl;
  
  /* parallel_for(intT i=0;i<numVertices-1;i++){ */
  /*   if(tmp[i] != tmp[i+1]) { */
  /*     Frontier[tmp[i]] = i+start; */
  /*   } */
  /* } */
  /* if(tmp[numVertices-1] != frontierSize) */
  /*   Frontier[frontierSize-1] = numVertices-1+start; */

  int round = 0;

  while (frontierSize > 0 ) {
    round++;
    //cout<<round<<" "<<frontierSize<<endl;
    parallel_for (intT i=0; i < frontierSize; i++) {
      int v = Frontier[i];
      Counts[i] = V[v].degree;
      long currVisitedV = currVisited[v];
      long currVisitedNgh;

      for (intT j=0; j < V[v].degree; j++) {
        int ngh = V[v].Neighbors[j];
	currVisitedNgh = currVisited[ngh];
	//check if neighbor still valid, if so then try to hook
	if(currVisitedNgh != (currVisitedNgh | currVisitedV))
	  if (hooks[ngh] > v)
	    utils::writeMin(&hooks[ngh],v);
      }
    }
    intT nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);
    work+=nr;
    //cout<<"Edges processed ="<<nr<<endl;
    /* parallel_for(intT i=0;i<frontierSize;i++) { */
    /*   int v = Frontier[i]; */
    /*   nextVisited[v] = nextVisited[v] | currVisited[v]; */
    /* } */
    parallel_for(intT i = 0; i < frontierSize; i++) {
      int v = Frontier[i];
      intT o = Counts[i];
      long currVisitedV = currVisited[v];
      long currVisitedNgh;
      for (intT j=0; j < V[v].degree; j++) {
        int ngh = V[v].Neighbors[j];
	currVisitedNgh = currVisited[ngh];
	if(currVisitedNgh == (currVisitedNgh | currVisitedV))
	  FrontierNext[o+j] = -1;
	else {
	  writeOr(&nextVisited[ngh],currVisitedV);
	  //if(getRadii)
	  if(lastChangedTime[ngh] != round)
	    lastChangedTime[ngh] = round;
	  
	  if(hooks[ngh] == v) {
	    FrontierNext[o+j] = ngh;
	    hooks[ngh] = INT_MAX;
	  }
	  else {
	    FrontierNext[o+j] = -1;
	  }
	}
      }
      writeOr(&nextVisited[v],currVisitedV);
    }
    swap(nextVisited,currVisited);
    frontierSize = sequence::filter(FrontierNext,Frontier,nr,notX(-1));
    

  }

  //cout<<"Total edges processed = "<<work<<endl;
  return work;
}


/* void BFS64_single(graph & A, intT n, intT m, intT start, */
/* 		  intT & frontierSize, */
/* 		  int* hooks, */
/* 		  int* Frontier, long* currVisited, long* nextVisited, */
/* 		  int* FrontierNext, intT* Counts, */
/* 		  int* lastChangedTime, int round) { */

/*   //intT work = 0; */
/*   intT numVertices = n; */
/*   intT numEdges = m; */
/*   vertex* V = A.V; */

/*   parallel_for (intT i=0; i < frontierSize; i++) { */
/*     int v = Frontier[i]; */
/*     Counts[i] = V[v].degree; */
/*     long currVisitedV = currVisited[v]; */
/*     long currVisitedNgh; */

/*     for (intT j=0; j < V[v].degree; j++) { */
/*       int ngh = V[v].Neighbors[j]; */
/*       currVisitedNgh = currVisited[ngh]; */
/*       //check if neighbor still valid, if so then try to hook */
/*       if(currVisitedNgh != (currVisitedNgh | currVisitedV)) */
/* 	if (hooks[ngh] > v) */
/* 	  utils::writeMin(&hooks[ngh],v); */
/*     } */
/*   } */
/*   intT nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0); */
/*   //  work+=nr; */
/*   //cout<<"Edges processed ="<<nr<<endl; */
/*   /\* parallel_for(intT i=0;i<frontierSize;i++) { *\/ */
/*   /\*   int v = Frontier[i]; *\/ */
/*   /\*   nextVisited[v] = nextVisited[v] | currVisited[v]; *\/ */
/*   /\* } *\/ */
/*   parallel_for(intT i = 0; i < frontierSize; i++) { */
/*     int v = Frontier[i]; */
/*     intT o = Counts[i]; */
/*     long currVisitedV = currVisited[v]; */
/*     long currVisitedNgh; */
/*     for (intT j=0; j < V[v].degree; j++) { */
/*       int ngh = V[v].Neighbors[j]; */
/*       currVisitedNgh = currVisited[ngh]; */
/*       if(currVisitedNgh == (currVisitedNgh | currVisitedV)) */
/* 	FrontierNext[o+j] = -1; */
/*       else { */
/* 	writeOr(&nextVisited[ngh],currVisitedV); */
/* 	if(lastChangedTime[ngh] != round) */
/* 	  lastChangedTime[ngh] = round; */
	  
/* 	if(hooks[ngh] == v) { */
/* 	  FrontierNext[o+j] = ngh; */
/* 	  hooks[ngh] = INT_MAX; */
/* 	} */
/* 	else { */
/* 	  FrontierNext[o+j] = -1; */
/* 	} */
/*       } */
/*     } */
/*     writeOr(&nextVisited[v],currVisitedV); */
/*   } */

/*   swap(nextVisited,currVisited); */

/*   frontierSize = sequence::filter(FrontierNext,Frontier,nr,notX(-1)); */
/*   //return frontierSize; */
/*   //cout<<"Total edges processed = "<<work<<endl; */
/*   //  return work; */
/* } */

//version with n , m and start as inputs
void BFS64_hybrid(graph& A, intT n, intT m, intT start,
		  int* hooks,
		  int* Frontier, long* currVisited, long* nextVisited,
		  int* FrontierNext, intT* Counts,
		  int* radii, int* flags,int K=64) {

  //intT work = 0;
  intT numVertices = n;
  intT numEdges = m;
  vertex* V = A.V;
  const intT threshold = m/1000;
  intT frontierSize = 0;
  /* int maxNum = min<intT>(numVertices,K); */
  /* for(intT i=start;i<start+numVertices;i++){ */
  /*   if(currVisited[i]) { */
  /*     Frontier[frontierSize++] = i; */
  /*     if(frontierSize >= maxNum) break; */
  /*   } */
  /* } */

  int round = 0;
  //int computeFrontier = 0;
  while (1) {
    //if(frontierSize==0)break;
    round++;

    //determine which method to use
    

    if(frontierSize < threshold){
      //need to compute frontier if switching from spmv to spmspv
      //cout<<"round "<<round<<" using spmspv . frontierSize = "<<frontierSize<<endl;

      frontierSize = 0;
      for(intT i=start;i<start+numVertices;i++){
	if(currVisited[i] != nextVisited[i]){
	  Frontier[frontierSize++] = i;
	}
      }
      

      //cout<<frontierSize<<endl;
      parallel_for (intT i=0; i < frontierSize; i++) {
	int v = Frontier[i];
	Counts[i] = V[v].degree;
	long currVisitedV = currVisited[v];
	long currVisitedNgh;

	for (intT j=0; j < V[v].degree; j++) {
	  int ngh = V[v].Neighbors[j];
	  currVisitedNgh = currVisited[ngh];
	  //check if neighbor still valid, if so then try to hook
	  if(currVisitedNgh != (currVisitedNgh | currVisitedV))
	    if (hooks[ngh] > v)
	      utils::writeMin(&hooks[ngh],v);
	}
      }
      intT nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0);
      //work+=nr;
      //cout<<"Edges processed ="<<nr<<endl;
      /* parallel_for(intT i=0;i<frontierSize;i++) { */
      /*   int v = Frontier[i]; */
      /*   nextVisited[v] = nextVisited[v] | currVisited[v]; */
      /* } */
      parallel_for(intT i = 0; i < frontierSize; i++) {
	int v = Frontier[i];
	intT o = Counts[i];
	long currVisitedV = currVisited[v];
	long currVisitedNgh;
	for (intT j=0; j < V[v].degree; j++) {
	  int ngh = V[v].Neighbors[j];
	  currVisitedNgh = currVisited[ngh];
	  if(currVisitedNgh == (currVisitedNgh | currVisitedV))
	    FrontierNext[o+j] = -1;
	  else {
	    writeOr(&nextVisited[ngh],currVisitedV);
	    //if(getRadii)
	    if(radii[ngh] != round)
	      radii[ngh] = round;
	  
	    if(hooks[ngh] == v) {
	      FrontierNext[o+j] = ngh;
	      hooks[ngh] = INT_MAX;
	    }
	    else {
	      FrontierNext[o+j] = -1;
	    }
	  }
	}
	writeOr(&nextVisited[v],currVisitedV);
      }
      frontierSize = sequence::filter(FrontierNext,Frontier,nr,notX(-1));
      if(!frontierSize) break;
      swap(nextVisited,currVisited);

    } else {
      /* cout<<"round "<<round<<" using spmv . frontierSize = "<< */
      /* 	frontierSize<<endl; */

      cilk_for(intT i=start;i<numVertices+start;i++){
	nextVisited[i] = currVisited[i];
	for(intT j=0;j<V[i].degree;j++){
	  intT ngh = V[i].Neighbors[j];
	  nextVisited[i] = nextVisited[i] | currVisited[ngh];
	}
      }

      memset(flags+start,0,numVertices*sizeof(int));
      //check if changed
      cilk_for(intT i=start;i<numVertices+start;i++){	
	if(currVisited[i] != nextVisited[i]){
	  radii[i] = round;
	  flags[i] = 1;
	} 
      }

      frontierSize = sequence::plusScan(flags,flags,n);
      if(!frontierSize) break;
    
      swap(currVisited,nextVisited);

      //compute frontier size once in a while so that can switch back to spmspv
      
    }    

  }

  //cout<<"Total edges processed = "<<work<<endl;
  //return work;
}



