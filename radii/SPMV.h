#include "sequence.h"
#include "gettime.h"
#include "graph.h"
#include "parallel.h"
#include <string.h>

void spmv_bitOR(graph& A, intT n, intT start,
		long* currVisited, long* nextVisited,
		int* radii, int* flags ) {
  vertex* V = A.V;
  int round = 0;

  //cout<<n<<endl;

  //starting frontier
  //for(intT i=start;i<start+n;i++)if(currVisited[i])cout<<i<<" ";cout<<endl;
  while(round++<n){ 
    cilk_for(intT i=start;i<start+n;i++){
      nextVisited[i] = currVisited[i];
      for(intT j=0;j<V[i].degree;j++){
	intT ngh = V[i].Neighbors[j];
	nextVisited[i] = nextVisited[i] | currVisited[ngh];
      }
    }
    memset(flags+start,0,n*sizeof(int));
    //check if changed
    cilk_for(intT i=start;i<start+n;i++){	
      if(currVisited[i] != nextVisited[i]){
	radii[i] = round;
	flags[i] = 1;
      } 
    }

    intT numChanged = sequence::plusScan(flags+start,flags+start,n);
    //cout<<round<<" "<<numChanged<<endl;
    if(!numChanged) break;

    swap(currVisited,nextVisited);
  }

}
