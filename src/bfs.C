#include <iostream>
#include <limits.h>
#include "sequence.h"
#include "graph.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

// **************************************************************
//    NONDETERMINISTIC BFS
// **************************************************************

struct nonNegF{bool operator() (int a) {return (a>=0);}};

pair<int,int> BFS_ND(vindex start, graph GA) {
  int numVertices = GA.n;
  int numEdges = GA.m;
  vertex *G = GA.V;
  vindex* Frontier = newA(vindex,numEdges);
  vindex* Level = newA(vindex,numVertices);
  vindex* FrontierNext = newA(vindex,numEdges);
  int* Counts = newA(int,numVertices);
  {cilk_for(int i = 0; i < numVertices; i++) Level[i] = -1;}

  Frontier[0] = start;
  int frontierSize = 1;
  Level[start] = 0;

  int totalVisited = 0;
  int round = 0;

  while (frontierSize > 0) {
    round++;
    totalVisited += frontierSize;

    {cilk_for (int i=0; i < frontierSize; i++) 
	Counts[i] = G[Frontier[i]].degree;}
    int nr = sequence::scan(Counts,Counts,frontierSize,utils::addF<int>(),0);

    // For each vertexB in the frontier try to "hook" unvisited neighbors.
    {cilk_for(int i = 0; i < frontierSize; i++) {
      int k= 0;
      vindex v = Frontier[i];
      int o = Counts[i];
      for (int j=0; j < G[v].degree; j++) {
        vindex ngh = G[v].Neighbors[j];
	if (Level[ngh] == -1) {
	  Level[ngh] = Level[v] + 1;
	  FrontierNext[o+j] = ngh;
	}
	else FrontierNext[o+j] = -1;
      }
      }}

    // Filter out the empty slots (marked with -1)
    frontierSize = sequence::filter(FrontierNext,Frontier,nr,nonNegF());
  }
  free(FrontierNext); free(Frontier); free(Counts); free(Level);
  return pair<int,int>(totalVisited,round);
}

// **************************************************************
//    THE DETERMINISTIC BSF
//    Updates the graph so that it is the BFS tree (i.e. the neighbors
//      in the new graph are the children in the bfs tree)
// **************************************************************

timer scanTime;
timer filterTime;
timer forLoop1Time;
timer forLoop2Time;

pair<int,int> BFS(vindex start, graph GA) {
  int numVertices = GA.n;
  int numEdges = GA.m;
  vertex *G = GA.V;
  int* Offsets = newA(int,numVertices+1);
  int* Parents = newA(int,numVertices);
  cilk_for (int i = 0; i < numVertices; i++) Parents[i] = INT_MAX;
  vindex* Frontier = newA(vindex,numVertices);
  vindex* FrontierNext = newA(vindex,numEdges);

  Frontier[0] = start;
  int fSize = 1;
  Parents[start] = -1;
  int round = 0;
  int totalVisited = 0;

  scanTime.clear();
  filterTime.clear();
  forLoop1Time.clear();
  forLoop2Time.clear();

  while (fSize > 0) {
    totalVisited += fSize;
    round++;

    forLoop1Time.start();
    // For each vertex in the frontier try to "hook" unvisited neighbors.
    cilk_for(int i = 0; i < fSize; i++) {
      int k= 0;
      vindex v = Frontier[i];
      for (int j=0; j < G[v].degree; j++) {
        vindex ngh = G[v].Neighbors[j];
	if (Parents[ngh] > v)
	  if (utils::writeMin(&Parents[ngh],v))
	    G[v].Neighbors[k++] = ngh;
      }
      Offsets[i] = k;
    }
    forLoop1Time.stop();

    scanTime.start();
    // Find offsets to write the next frontier for each v in this frontier
    int nr=sequence::scan(Offsets,Offsets,fSize,utils::addF<int>(),0);
    Offsets[fSize] = nr;
    scanTime.stop();

    // Move hooked neighbors to next frontier.   
    forLoop2Time.start();
    cilk_for (int i = 0; i < fSize; i++) {
      int o = Offsets[i];
      int d = Offsets[i+1]-o;
      int k = 0;
      vindex v = Frontier[i];
      for (int j=0; j < d; j++) {
	vindex ngh = G[v].Neighbors[j];
	if (Parents[ngh] == v) {
	  FrontierNext[o+j] = G[v].Neighbors[k++] = ngh;
	  Parents[ngh] = -1;
	}
	else FrontierNext[o+j] = -1;
      }
      G[v].degree = k;
    }
    forLoop2Time.stop();

    filterTime.start();
    // Filter out the empty slots (marked with -1)
    fSize = sequence::filter(FrontierNext,Frontier,nr,nonNegF());
    filterTime.stop();
  }

  cout << "For loop 1 time: " << forLoop1Time.total() << endl;
  cout << "Scan time: " << scanTime.total() << endl;
  cout << "For loop 2 time: " << forLoop2Time.total() << endl;
  cout << "Filter time: " << filterTime.total() << endl;

  free(FrontierNext); free(Frontier); free(Offsets); free(Parents);
  return pair<int,int>(totalVisited,round);
}
