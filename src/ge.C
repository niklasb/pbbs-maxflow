#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "graph.h"
#include "cilk.h"
#include "quickSort.h"
#include <math.h>
#include <vector>
#include <fstream>
using namespace std;

timer mm;

//Return memory size in bytes
long memorySize() {
  long mem;
  string token;
  ifstream file("/proc/meminfo",ios::in);
  while(file >> token) {
    if(token == "MemTotal:") {
      if(!(file >> mem)) {
	cout<<"Error reading memory size\n";
	return 0;       
      }
      break;
    }
  }
  file.close();
  return mem*1000;
}

void gaussElim(vindex* vtx, int n, vertex* G, int m){
  int* reserve = newA(int,n);
  cilk_for(int i=0;i<n;i++) reserve[i]=-1;

  int end = n;
  int wasted = 0;
  int ratio = 100;
  int maxR = 1 + n/ratio;
  int round = 0;
  vindex *hold = newA(vindex,maxR); //temp array for pack
  bool *keep = newA(bool,maxR); //0 if win, 1 if lose
  //vindex *vtx = vertices.S;
  ulong spaceUsed = 0;
  //int totalDegree = 2*m;
  //float density = totalDegree/end;
  //float threshold = 10*sqrt(n);
  long fill = 0;
  long ops = 0;

  vector<int*> allocatedMemory;
  long memory = memorySize()/10; //specific to machine
  int* numNghs = newA(int,end);
  while(end > 0 /*&& density < threshold*/){
    //****MEMORY CLEANUP IF NECESSARY****
    mm.start();
    if(spaceUsed > memory) { 
      //copy all edges of alive vertices to new array
      //and free allocated memory
      //int* numNghs = newA(int,end);
      cilk_for(int i=0;i<end;i++){
	vindex v = vtx[i];
	numNghs[i] = G[v].degree;
      }
      int memNeeded = sequence::plusScan(numNghs,numNghs,end);
      int* newSpace = newA(vindex,memNeeded);
      //copy to new memory
      cilk_for(int i=0;i<end;i++){
	vindex v = vtx[i];
	int offset = numNghs[i];
	cilk_for(int j=0;j<G[v].degree;j++){
	  newSpace[offset+j] = G[v].Neighbors[j];
	}
	G[v].Neighbors = newSpace + offset;
      }

      //free(numNghs);

      //free old memory
      cilk_for(int i=0;i<allocatedMemory.size();i++){
	free(allocatedMemory[i]);
      }
      allocatedMemory.clear();
      //add new memory to allocatedMemory
      allocatedMemory.push_back(newSpace);
      spaceUsed = memNeeded;
    }
    mm.stop();
    round++;
    int size = min(maxR,end); 
    int start = end - size;

    //****RESERVE VERTICES****
    //reserve self and neighbors
    cilk_for(int i=0;i<size;i++){ 
      vindex v = vtx[start+i];
      hold[i] = v;
      utils::writeMax(&reserve[v],v); //write to self
      for(int j=0;j<G[v].degree;j++){ //write to neighbors
	vindex ngh = G[v].Neighbors[j];
	utils::writeMax(&reserve[ngh],v);
      }
    }

    //****FIND WINNERS****
    //check if won, and compute number of neighbors
    //int* numNghs = newA(int,size);
    cilk_for(int i = 0;i<size;i++){
      vindex v = vtx[start+i];
      keep[i] = (v != reserve[v]); //0 if win, 1 if lose
      if(!keep[i]){
	for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j];
	  if((keep[i] = (v != reserve[ngh])))
	    break;
	}
      }
      if(!keep[i]){
	numNghs[i] = G[v].degree;
      } 
      else numNghs[i] = 0;
      
    }

    //****MEMORY ALLOCATION****
    //compute total number of neighbors of winners
    int s = sequence::plusScan(numNghs,numNghs,size);
    //compute total number of neighbors of winners and 
    //the number of neighbors of the winners' neighbors
    int* numNghsNghs = newA(int,s);
    cilk_for(int i=0;i<size;i++){
      if(!keep[i]){
	vindex v = vtx[start+i];
	int offset = numNghs[i];
	cilk_for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j];
	  //set max new degree 
	  numNghsNghs[offset+j] = 
	    min(G[ngh].degree+G[v].degree-2,end-1);
	}
      }
    }
    int t = sequence::plusScan(numNghsNghs,numNghsNghs,s);
    //allocate memory for new neighbors
    vindex* space = newA(vindex,t);
    spaceUsed += t;
    allocatedMemory.push_back(space);


    //****BUILD CLIQUES****
    int* numFill = newA(int, s);
    int* numOps = newA(int,s);
    //if won, remove self and make clique of neighbors
    cilk_for(int i=0;i<size;i++){
      if(!keep[i]){ //check if won
	vindex v = vtx[start+i];
	//maybe make this loop not parallel?
	cilk_for(int j=0;j<G[v].degree;j++){
	  vindex ngh = G[v].Neighbors[j];
	  int a = G[ngh].degree;
	  int b = G[v].degree;
	  int offset = numNghsNghs[numNghs[i]+j];
	  vindex* newNeighbors = space + offset;

	  //merge adjacency lists
	  //remove duplicates, self edges, and edge to winner
	  int ai = 0; int bi = 0; int k = 0;
	  while(1){
	    if(ai >= a && bi >=b) break;
	    //skipping self edges and edges to winner	     
	    else if(ai < a && G[ngh].Neighbors[ai] == v){
	      ai++;
	    }
	    else if(bi < b && G[v].Neighbors[bi] == ngh){
	      bi++;
	    }
	    //merge
	    else if(ai >= a) { //a is empty
	      newNeighbors[k++] = G[v].Neighbors[bi++];
	    } else if(bi >= b) { //b is empty 
	      newNeighbors[k++] = G[ngh].Neighbors[ai++];
	    } else { //both not empty
	      vindex aval = G[ngh].Neighbors[ai];
	      vindex bval = G[v].Neighbors[bi];
	      if(aval < bval) newNeighbors[k++] = G[ngh].Neighbors[ai++];
	      else if(bval < aval) newNeighbors[k++] = G[v].Neighbors[bi++];
	      else { //equal case (removing duplicate edge)
		newNeighbors[k++] = G[ngh].Neighbors[ai++]; 
		bi++;
	      }
	    }
	  }
	  //update number of new edges and degrees
	  numFill[numNghs[i]+j] = 1+k-G[ngh].degree;
	  numOps[numNghs[i]+j] = k;
	  G[ngh].degree = k;
	  G[ngh].Neighbors = newNeighbors;
	}
	//numNewDegree[numNghs[i]] -= G[v].degree;
      }
    }
    fill += sequence::plusScan(numFill,numFill,s);
    ops += sequence::plusScan(numOps,numOps,s);


    //free(numNghs);
    free(numNghsNghs);
    free(numFill); free(numOps);
 
    //****PHASE CLEANUP****
    //pack out winners, reset reserves, update number of edges
    //and density
    int nn = sequence::pack(hold,vtx+start,keep,size);
    end = start + nn;
    wasted += nn;
    cilk_for(int i=0;i<end;i++) {
      if(reserve[vtx[i]] != -1)
	reserve[vtx[i]]=-1;
    }
    //totalDegree += numNew;
    //if(end) density = totalDegree/end;
  }
  free(keep);
  free(hold);
  free(numNghs);
  /*
  if(end){ 
    cout<<"Terminated early because graph became too dense"<<endl;
    cout<<"Vertices remaining = "<<end<<endl;
    cout<<"Edges remaining = "<<totalDegree/2<<endl;
    }*/
  cout << "Fill : " << fill << endl;
  cout << "Num ops : " << ops << endl;
}


void ge(graph G) {
  int n = G.n;
  //seq<vindex> vertices = seq<vindex>(n, utils::identityF<int>());
  vindex* vertices = newA(vindex,n);
  cilk_for(int i=0;i<n;i++)vertices[i]=i;
  vertex* vtx = G.V;
  //sort neighbors in adjacency list by id
  cilk_for(int i=0;i<n;i++){
    quickSort(vtx[i].Neighbors,vtx[i].degree,less<int>());    
  }
  
  gaussElim(vertices,n,vtx,G.m);
  cout<<"Time spent freeing wasted memory and copying: "<<mm.total()<<endl;
  //vertices.del();
  free(vertices);
}
