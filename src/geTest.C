#include <iostream>
#include <cstdlib>
#include "sequence.h"
#include "graphGen.h"
#include "gettime.h"
#include "cilk.h"
#include "graph.h"
#include "seq.h"
#include "math.h"
#include "string.h"
using namespace std;

void ge(graph G);
//void ge(graph G, int* levels);
// **************************************************************
//    MAIN
// **************************************************************

int cilk_main(int argc, char *argv[]) {
  int n,m;
  if (argc > 1) n = atoi(argv[1]); else n = 10;
  if (argc > 2) m = atoi(argv[2]); else m = 10;
  /*
  {
    graph G = graphRandomWithDimension(2, m, n);
    cout << "m = " << G.m << endl;
    G = graphReorder(G,NULL);
    startTime();    
    ge(G,0);
    stopTime(.1,"Gaussian elimination (random)");
    
    G.del(); 
  }
  */
  {
    //graph G = graph2DMeshPlanar(n);
    graph G = graph2DMesh(n);

    int n = G.n; cout<<"n = "<<n<<endl;
    vertex* vtx = G.V;
    /*
    for(int i=n-1;i>=0;i--){
      cout<<i<<" :";
      for(int j=0;j<vtx[i].degree;j++){
      	cout<<vtx[i].Neighbors[j]<<" ";
      }
      cout<<endl;
      }
    */
    bool* flags = newA(bool,n);
    cilk_for(int i=0;i<n;i++)flags[i]=0;

    int* levels = newA(int,n);
    int* ordering = newA(int,n);
    int rootn = sqrt(n);
    int logrootn = ceil(log2(rootn));
    int mm = n-1;
    for(int i=2;i<logrootn+1;i++){
      int size = min(1 << i,rootn);
      for(int j=0;j<=rootn-size;j=j+size){
	for(int h=0;h<=rootn-size;h=h+size){
	  //add internal nodes to ordering
	  for(int k=1;k<size-1;k++){
	    for(int l=1;l<size-1;l++){
	      int offset = (j+k)*rootn+(l+h);
	      if(!flags[offset]){
		levels[mm] = i;
		ordering[mm--] = offset;
		flags[offset] = 1;
	      }
	    }
	  }
	}
      }
    }
    for(int i=0;i<n;i++){
      if(!flags[i]) {
	levels[mm] = logrootn+1;
	ordering[mm--] = i;
      }
    }
    free(flags);
    //for(int i=0;i<n;i++)cout<<ordering[i]<<" ";cout<<endl;
    int* inverse = newA(int,n);
    cilk_for(int i=0;i<n;i++) inverse[ordering[i]] = i;

    //for(int i=0;i<n;i++) cout<<levels[i]<<" ";cout<<endl;
    //int* reverse = newA(int,n);
    //for(int i=0;i<n;i++) reverse[i] = ordering[n-i-1];
    //for(int i=0;i<n;i++)cout<<inverse[i]<<" ";cout<<endl;
    G = graphReorder(G,inverse);
    //G = graphReorder(G,0);
    free(ordering); free(inverse);
    /*vtx = G.V;
    for(int i=0;i<n;i++){
      cout<<i<<" :"; cout<<vtx[i].degree<<" ,";
      for(int j=0;j<vtx[i].degree;j++){
      	cout<<vtx[i].Neighbors[j]<<" ";
      }
      cout<<endl;
      }*/

    startTime();
    ge(G);
    stopTime(.1,"Gaussian elimination (2dmesh)");
    G.del(); 
  }

  reportTime("Gaussian elimination (weighted average)");

}

