#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "graph.h"
#include "cilk.h"
#include "serialSort.h"
#include <math.h>
#include <string.h>
#include "gettime.h"
using namespace std;


void gaussElim(int n, vertex* G, int m){
  //int totalDegree = 2*m;
  //float density = totalDegree/n;
  //float threshold = 10*sqrt(n);
  
  //keep track of which vertices are using new memory,
  //which is freeable
  bool* hasNew = newA(bool,n); 
  memset(hasNew,0,n);
  
  //testing
  int fill=0;
  long numOps=0;

  //remove self and make clique of neighbors
  //run loop in reverse to be consistent with parallel version
  for(int i=n-1;i>=0;i--){
    vindex v = i;
    /*
    if(density > threshold) {
      cout<<"Terminating early because graph became too dense"<<endl;
      cout<<"Number of vertices remaining: "<<i+1<<endl;
      cout<<"Number of edges remaining: "<<totalDegree/2<<endl;
      break;
      }*/
    int b = G[v].degree;
    //int numNew = 0;
    //looping through neighbors
    for(int j=0;j<b;j++){
      vindex ngh = G[v].Neighbors[j];
      int a = G[ngh].degree;
      int maxNewDegree = min(a+b-2,i);
      vindex* newNeighbors = newA(vindex,maxNewDegree);
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
	} 
	else if(bi >= b) { //b is empty 
	  newNeighbors[k++] = G[ngh].Neighbors[ai++];
	} 
	else { //both not empty
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
      //update number of new edges and new degrees
      int newEdges = k-G[ngh].degree;
      //numNew += newEdges;
      fill+=1+newEdges;
      G[ngh].degree = k;
      if(hasNew[ngh]){ //vertex currently using freeable memory
	free(G[ngh].Neighbors);
      }
      else hasNew[ngh] = 1;
      
      G[ngh].Neighbors = newNeighbors;
      numOps+=k;
    }
    //update number of new edges and density
    //totalDegree += numNew - G[v].degree;
    //if(i) density = totalDegree/i;
  }
  free(hasNew);
  cout<<"Fill: "<<fill<<endl;
  cout<<"NumOps: "<<numOps<<endl;
}


void ge(graph G) {
  int n = G.n;
  vertex* vtx = G.V;

  //sort neighbors in adjacency list by id
  for(int i=0;i<n;i++){
    compSort(vtx[i].Neighbors,vtx[i].degree,less<int>());
  }
  gaussElim(n,vtx,G.m);
}
