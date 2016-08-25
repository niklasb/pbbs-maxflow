#include "ligra.h"
#include "gettime.h"
using namespace std;

struct BF_F {
  int* ShortestPathLen;
  int* Visited;
  BF_F(int* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool update (intT s, intT d, intT edgeLen) {
    intT newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool updateAtomic (intT s, intT d, int edgeLen){
    int newDist = ShortestPathLen[s] + edgeLen;
    return (writeMin(&ShortestPathLen[d],newDist) &&
	    CAS(&Visited[d],0,1));
  }
  inline bool cond (intT d) { return 1; }
};

struct BF_Vertex_F {
  int* Visited;
  BF_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (intT i){
    Visited[i] = 0;
    return 1;
  }
};

timer t1,t2;

template <class vertex>
int* BellmanFord(intT start, wghGraph<vertex> GA) {
  
  startTime();
  bool denseForward = 1;
  intT n = GA.n;

  int* ShortestPathLen = newA(int,n);
  parallel_for(intT i=0;i<n;i++) ShortestPathLen[i] = INT_MAX/2;
  ShortestPathLen[start] = 0;

  int* Visited = newA(int,n);
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;

  vertices Frontier(n,start);

  intT round = 0;
  timer t1;
  
  while(!Frontier.isEmpty()){
    round++;
    if(round == n) {
      //negative weight cycle
      parallel_for(intT i=0;i<n;i++) ShortestPathLen[i] = -(INT_MAX/2);
      break;
    }
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;

    t1.start();
    vertices output = edgeMap(GA, Frontier, BF_F(ShortestPathLen,Visited), GA.m/20,denseForward);
    t1.stop();
    t2.start();
    vertexMap(output,BF_Vertex_F(Visited));
    t2.stop();
    Frontier.del();
    Frontier = output;
  } 

  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t2.reportTotal("reset");
  t1.reportTotal("MVMult");
  Frontier.del();
  free(Visited);
  nextTime("Total");
  return ShortestPathLen;
}


int parallel_main(int argc, char* argv[]) {  
  char* iFile;
  bool binary = false;
  bool symmetric = false;
  if(argc > 1) iFile = argv[1];
  if(argc > 2) if((string) argv[2] == (string) "-s") symmetric = true;
  if(argc > 3) if((string) argv[3] == (string) "-b") binary = true;
  
  if(symmetric) {
    wghGraph<symmetricWghVertex> WG = 
      readWghGraph<symmetricWghVertex>(iFile,symmetric,binary);
    BellmanFord((intT)1,WG);
    WG.del(); 
  } else {
    wghGraph<asymmetricWghVertex> WG = 
      readWghGraph<asymmetricWghVertex>(iFile,symmetric,binary);
    BellmanFord((intT)1,WG);
    WG.del();
  }
}
