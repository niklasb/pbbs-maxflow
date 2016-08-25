#include "graph.h"
#include "parallel.h"
#include "Diameter.h"
#include "sequence.h"
#include "gettime.h"
#include "BFS.h"
#include "ST.h"
#include "SPMV.h"
#include <string.h>
#include <stdio.h>
using namespace std;

bool moreSecond(const intPair & p, const intPair & q){
  if(p.second > q.second) return 1;
  else if(p.second < q.second) return 0;
	  else return (p.first < q.first);
}

int* allBFS(graph A){
  intT n = A.n;
  intT m = A.m;
  if(n <= 0) abort();
  startTime();

  int* radii = newA(int,n);
  intT* Frontier = newA(intT,m);
  intT* Visited = newA(intT,n);
  intT* FrontierNext = newA(intT,m);
  intT* Counts = newA(intT,n);

  intT diameter = 0;
  intT result;
  for(intT j=0;j<n;j++){    
    result = BFS(j,A,Frontier,Visited,FrontierNext,Counts);
    radii[j] = result;
    if(result > diameter) diameter = result;   
  }
  free(FrontierNext); free(Frontier); free(Counts); free(Visited);

  cout << "Diameter = " << diameter << endl;
  nextTime();
  return radii;
}

void kBFS(graph A, int K){
  intT n = A.n;
  intT m = A.m;
  intT* Frontier = newA(intT,m);
  intT* Visited = newA(intT,n);
  intT* FrontierNext = newA(intT,m);
  intT* Counts = newA(intT,n);

  startTime();
  for(int i=0;i<K;i++){
    BFS(i,A,Frontier,Visited,FrontierNext,Counts);
  }

  nextTime();
  free(Frontier); free(Visited); free(FrontierNext); free(Counts);
}

// void propagate(graph A, intT compSize, intT numEdges, intT start,
// 	       int* hooks,
// 	       int* Frontier, long* currVisited, long* nextVisited,
// 	       int* FrontierNext, intT* Counts,
// 	       int* radii, int K=64) {

//   //need to have functions that perform single iterations of BFS and spmv
//   intT frontierSize = 0;
//   int maxNum = min<intT>(compSize,K);
//   for(intT i=start;i<start+compSize;i++){
//     if(currVisited[i]) {
//       Frontier[frontierSize++] = i;
//       if(frontierSize >= maxNum) break;
//     }
//   }

//   int round = 0;

//   while(1){
//     round++;
//   //if switching from spmv to spsv, need to compute frontier

//   //for spmv, quit if no bit vectors change
//   //for spsv, quit if frontier size = 0
//     if(true){
//       while(frontierSize > 0){
// 	 cout<<frontierSize<<endl;
// 	 //if(frontierSize == 0) break;
//       BFS64_single(A,compSize,numEdges,start,frontierSize,hooks,Frontier,currVisited,nextVisited,FrontierNext,Counts,radii,round);
//       }
//     }
//     else{
//       int* flags = (int*) Counts;
//       spmv_bitOR_single(A,compSize,currVisited,nextVisited,start);
//     }
//   }

// }

void processComps_hybrid(graph A, component comp,int* radii,int K=64){

  intT n = A.n;
  intT m = A.m;

  int numComp = comp.numComp;
  uint* compStarts = comp.compStarts;

  int* Frontier = newA(int,m);
  long* currVisited = newA(long,n);
  long* nextVisited = newA(long,n);
  int* FrontierNext = newA(int,m);
  intT* Counts = newA(intT,n);
  // uint* furthest = newA(uint,n);
  // uint* addFurthest = newA(uint,n);
  int* hooks = newA(int,n);
  parallel_for (intT i = 0; i < n; i++) hooks[i] = INT_MAX;
  
  int* flags = newA(int,n); //temp


  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));

  //get sum of degrees per component, used to add offset to arrays
  intT* degreeOffsets = newA(intT,n+1);
  degreeOffsets[n] = m;
  cilk_for(intT i=0;i<n;i++) degreeOffsets[i] = A.V[i].degree;

  sequence::plusScan(degreeOffsets,degreeOffsets,n);

  // intT* workCount = newA(intT,numComp);
  // memset(workCount,0,numComp*sizeof(intT));
  startTime();
  //assign radius of 0 to all singletons, and 1 to all 2-node components,
  //and do BFS on the remaining components
  parallel_for(intT i=0;i<numComp;i++){
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    intT compSize = end-start;
    if(compSize == 1)  radii[start] = 0; 
    else if(compSize == 2) 
      radii[start] = radii[start+1] = 1;
    else { //BFS

      for(int j=0;j<min<intT>(compSize,K);j++) {
   	currVisited[start+j] = (long) 1<<j;
  	radii[start+j] = 0;
      }
      intT degreeOffset = degreeOffsets[start];
      intT numEdges = degreeOffsets[end]-degreeOffset;

      
      BFS64_hybrid(A,compSize,numEdges,start,hooks,Frontier+degreeOffset,currVisited,nextVisited,FrontierNext+degreeOffset,Counts+start,radii,flags,K);


    }
  } 
  // intT totalWork = sequence::plusScan(workCount,workCount,numComp);
  // cout<<"number of edges visited = "<<totalWork<<endl;
  // cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("First BFS");
  
  intPair* B = newA(intPair,n);
  cilk_for(intT i=0;i<n;i++) {
    B[i].first = i; B[i].second = radii[i];
  }

  int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;
  

  //round 2 of BFS
  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));
  parallel_for (intT i = 0; i < n; i++) hooks[i] = INT_MAX;

  // memset(workCount,0,numComp*sizeof(intT));

  parallel_for(intT i=0;i<numComp;i++){
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    //cout<<start<<" "<<end<<endl;
    intT compSize = end-start;
    if(compSize > 64) {
      quickSort(B+start,compSize,moreSecond);

      //BFS
      for(int j=0;j<min<intT>(compSize,K);j++) {
   	currVisited[B[start+j].first] = (long) 1<<j;
  	radii[B[start+j].first] = 0;
      }
      intT degreeOffset = degreeOffsets[start];
      intT numEdges = degreeOffsets[end]-degreeOffset;
      
      BFS64_hybrid(A,compSize,numEdges,start,hooks,Frontier+degreeOffset,currVisited,nextVisited,FrontierNext+degreeOffset,Counts+start,radii,flags,K);

    }
  } 
  
  // totalWork = sequence::plusScan(workCount,workCount,numComp);
  // cout<<"number of edges visited = "<<totalWork<<endl;
  // cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("Second BFS");

  //  free(workCount);
  free(degreeOffsets);
  free(FrontierNext); free(Frontier); free(Counts); free(nextVisited); free(currVisited);
  free(hooks);
  //free(furthest); free(addFurthest);
  free(B);
  free(flags);
}

int* BFS_hybrid(graph& A, int K=64){
  intT n = A.n;
  intT m = A.m;
  if(n <= 0) abort();

  int* radii = newA(int,n);
  memset(radii,0,n*sizeof(int));
  startTime();
  component comp = st(A);
  
  //nextTime("connected components");
  
  processComps_hybrid(A,comp,radii,K);

  comp.del();

  //for(intT i=0;i<n;i++)cout<<radii[i]<<" ";cout<<endl;
  int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;

  return radii;
}


void processComps(graph A, component comp,int* radii,int K=64){

  intT n = A.n;
  intT m = A.m;

  int numComp = comp.numComp;
  uint* compStarts = comp.compStarts;

  int* Frontier = newA(int,m);
  long* currVisited = newA(long,n);
  long* nextVisited = newA(long,n);
  int* FrontierNext = newA(int,m);
  intT* Counts = newA(intT,n);
  // uint* furthest = newA(uint,n);
  // uint* addFurthest = newA(uint,n);
  int* hooks = newA(int,n);
  parallel_for (intT i = 0; i < n; i++) hooks[i] = INT_MAX;


  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));

  //get sum of degrees per component, used to add offset to arrays
  intT* degreeOffsets = newA(intT,n+1);
  degreeOffsets[n] = m;
  cilk_for(intT i=0;i<n;i++) degreeOffsets[i] = A.V[i].degree;

  sequence::plusScan(degreeOffsets,degreeOffsets,n);

  intT* workCount = newA(intT,numComp);
  memset(workCount,0,numComp*sizeof(intT));
  startTime();
  //assign radius of 0 to all singletons, and 1 to all 2-node components,
  //and do BFS on the remaining components
  parallel_for(intT i=0;i<numComp;i++){
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    intT compSize = end-start;
    if(compSize == 1)  radii[start] = 0; 
    else if(compSize == 2) 
      radii[start] = radii[start+1] = 1;
    else { //BFS
      
      for(int j=0;j<min<intT>(compSize,K);j++) {
   	currVisited[start+j] = (long) 1<<j;
  	radii[start+j] = 0;
      }
      intT degreeOffset = degreeOffsets[start];
      intT numEdges = degreeOffsets[end]-degreeOffset;

      workCount[i]=
  	BFS64(A,compSize,numEdges,start,hooks,Frontier+degreeOffset,currVisited,nextVisited,FrontierNext+degreeOffset,Counts+start,radii,K);

    }
  } 
  intT totalWork = sequence::plusScan(workCount,workCount,numComp);
  cout<<"number of edges visited = "<<totalWork<<endl;
  cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("First BFS");
  
  intPair* B = newA(intPair,n);
  cilk_for(intT i=0;i<n;i++) {
    B[i].first = i; B[i].second = radii[i];
  }

 int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;
   

  //round 2 of BFS
  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));
  parallel_for (intT i = 0; i < n; i++) hooks[i] = INT_MAX;

  memset(workCount,0,numComp*sizeof(intT));

  parallel_for(intT i=0;i<numComp;i++){
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    //cout<<start<<" "<<end<<endl;
    intT compSize = end-start;
    if(compSize > 64) {
      quickSort(B+start,compSize,moreSecond);

      //BFS
      for(int j=0;j<min<intT>(compSize,K);j++) {
   	currVisited[B[start+j].first] = (long) 1<<j;
  	radii[B[start+j].first] = 0;
      }
      intT degreeOffset = degreeOffsets[start];
      intT numEdges = degreeOffsets[end]-degreeOffset;
 
      workCount[i] = BFS64(A,compSize,numEdges,start,hooks,Frontier+degreeOffset,currVisited,nextVisited,FrontierNext+degreeOffset,Counts+start,radii,K);
    }
  } 
  
  totalWork = sequence::plusScan(workCount,workCount,numComp);
  cout<<"number of edges visited = "<<totalWork<<endl;
  cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("Second BFS");

  free(workCount);
  free(degreeOffsets);
  free(FrontierNext); free(Frontier); free(Counts); free(nextVisited); free(currVisited);
  free(hooks);
  //free(furthest); free(addFurthest);
  free(B);

}

int* sampleBFS(graph& A, int K=64){
  intT n = A.n;
  intT m = A.m;
  if(n <= 0) abort();

  int* radii = newA(int,n);
  memset(radii,0,n*sizeof(int));
  startTime();
  component comp = st(A);
  
  //nextTime("connected components");
  
  processComps(A,comp,radii,K);

  comp.del();

  //for(intT i=0;i<n;i++)cout<<radii[i]<<" ";cout<<endl;
  int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;

  return radii;
}


//typedef pair<int,int> intPair;



void processComps_BFS_spmv(graph& A,component comp,int* radii,	   
			   int K=64){
  intT n = A.n;
  intT m = A.m;
  vertex* V = A.V;
  int round = 0;

  long* currVisited = newA(long,n);
  long* nextVisited = newA(long,n);
  memset(currVisited,0,sizeof(long));
  memset(nextVisited,0,sizeof(long));

  int* flags = newA(int,n);

  int numComp = comp.numComp;
  uint* compStarts = comp.compStarts;

  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));
  
  for(int j=0;j<min<intT>(n,K);j++) {
    currVisited[j] = (long) 1<<j;
  }
  //intT* workCount = newA(intT,numComp);
  //memset(workCount,0,numComp*sizeof(intT));
  
  startTime();
  //assign radius of 0 to all singletons, and 1 to all 2-node components,
  //and do BFS on the remaining components
  cilk_for(intT i=0;i<numComp;i++){ 
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    intT compSize = end-start;
    if(compSize == 1)  radii[start] = 0; 
    else if(compSize == 2) 
      radii[start] = radii[start+1] = 1;
    else { //spmv
      for(int j=0;j<min<intT>(compSize,K);j++) {
   	currVisited[start+j] = (long) 1<<j;
	radii[start+j] = 0;
      }
      
      spmv_bitOR(A,compSize,start,currVisited,nextVisited,radii,flags);

    }
  } 

  // intT totalWork = sequence::plusScan(workCount,workCount,numComp);
  // cout<<"number of edges visited = "<<totalWork<<endl;
  // cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("First spmv");

  int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;

  intPair* B = newA(intPair,n);
  cilk_for(intT i=0;i<n;i++) {
    B[i].first = i; B[i].second = radii[i];
  }

  //nextTime("Finding furthest vertices");


  //round 2 of BFS
  memset(currVisited,0,n*sizeof(long));
  memset(nextVisited,0,n*sizeof(long));

  cilk_for(intT i=0;i<numComp;i++){
    intT start = compStarts[i];
    intT end = compStarts[i+1];
    intT compSize = end-start;
    if(compSize > max(K,2)) {
      quickSort(B+start,compSize,moreSecond);
      //BFS
      for(int j=0;j<min<intT>(compSize,K);j++) {
 	currVisited[B[start+j].first] = (long) 1<<j;
	radii[B[start+j].first] = 0;
      }
      spmv_bitOR(A,compSize,start,currVisited,nextVisited,
		 radii+start,flags+start);
 
    }
  } 
  
  // totalWork = sequence::plusScan(workCount,workCount,numComp);
  // cout<<"number of edges visited = "<<totalWork<<endl;
  // cout<<"work efficiency (K*m/work) = "<<K*m/(double)totalWork<<endl;
  nextTime("Second spmv + sorting");

  free(nextVisited); free(currVisited);
  free(flags);
  free(B);
}



//purely sparse matrix vector multiply
int* BFS_spmv(graph& A, int K=64){
  intT n = A.n;
  intT m = A.m;

  int* radii = newA(int,n);
  memset(radii,0,n*sizeof(int));
  startTime();
  component comp = st(A);

  //nextTime("connected components");
  
  processComps_BFS_spmv(A,comp,radii,K);

  comp.del();

  //for(intT i=0;i<n;i++)cout<<radii[i]<<" ";cout<<endl;
  int diameter = sequence::reduce(radii,n,utils::maxF<intT>());
  cout<<"Diameter = "<<diameter<<endl;

  return radii;

}

int* sampleBitStrings(graph& A, int K){
  
  const intT n = A.n;
  const intT m = A.m;
  vertex* V = A.V;
  if(n <= 0) { cout << "No edges in graph\n"; return 0;}
 
  int* radii = newA(int,n);
  memset(radii,0,n*sizeof(int));
 
  //create bitstrings
  const intT total = K*n;
  intT* prevBitStrings = newA(intT,total);
  cilk_for(intT i=0;i<total;i++){
    intT rand = utils::hash(i);
    intT bitString = 1;
    while(!(bitString & rand)) bitString <<= 1;
    prevBitStrings[i] = bitString;
  }
  
  intT* currBitStrings = newA(intT,total);
  intT* flags = newA(intT,n);
  
  const intT diamLimit = n;
  
  bool changed = 1;
  for(intT j=0;j<diamLimit;j++){
    //intT* starts = A.Starts; intT *cols = A.ColIds;
    cilk_for (intT i=0; i < n; i++) {
      intT iTimesK = i*K;
      for(intT k=0;k<K;k++){
	intT offset = iTimesK+k;
	currBitStrings[offset] = prevBitStrings[offset];
      }
      for(intT h=0;h<V[i].degree;h++){
	intT ngh = V[i].Neighbors[h];
	intT nghTimesK = ngh*K;
	
	//OR bit strings
	for(intT k=0;k<K;k++)
	  currBitStrings[iTimesK+k] |= prevBitStrings[nghTimesK+k];
	
      }
    }

    memset(flags,0,n*sizeof(intT));
    //check if converged
    cilk_for(intT i=0;i<n;i++){
      intT iTimesK = i*K;
      for(intT k=0;k<K;k++){
	intT offset = iTimesK+k;
	if(currBitStrings[offset] != prevBitStrings[offset]){
	  radii[i] = j;
	  flags[i] = 1;
	  break;
	}
      }
    }
    // for(intT i=0;i<total;i++){
    //   if(currBitStrings[i] != prevBitStrings[i]){
    // 	changed = 1;
    // 	break;
    //   }
    // }
    
    if(!sequence::plusScan(flags,flags,n)) {cout << "Diameter = " << j << endl; break;}

    //swap prev and curr
    swap(prevBitStrings,currBitStrings);
    
  }
  free(currBitStrings); free(prevBitStrings);
  return radii;
}

//Takes a symmetric graph and computes the radii/diameter
int* computeDiameter(graph& A, int option, int K){

  int* lastChangedTime;
  cout<<"K = "<<K<<" ";

  switch(option) {
    
  case 0:
    cout << "sampleBFS\n";
    lastChangedTime = sampleBFS(A,K);
    break;
  case 1:
    cout << "allBFS\n";
    lastChangedTime = allBFS(A);
    break;
  case 2:
    cout<< "sampleBitStrings\n";
    lastChangedTime = sampleBitStrings(A,K);
    break;
  case 3:
    cout<< "kBFS\n";
    kBFS(A,K);
    break;
  case 4:
    cout<< "BFS_spmv\n";
    lastChangedTime = BFS_spmv(A,K);
    break;
  case 5:
    cout<<"hybrid\n";
    lastChangedTime = BFS_hybrid(A,K);
    break;
  default:
    cout << "invalid function\n";
    break;
  }
  return lastChangedTime;
}
