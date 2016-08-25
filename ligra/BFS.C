#include "ligra.h"
#include "parallel.h"

using namespace std;

struct BFS_F {
  intT* Parents;
  BFS_F(intT* _Parents) : Parents(_Parents) {}
  inline bool update (intT s, intT d) {
    if(Parents[d] == -1) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool updateAtomic (intT s, intT d){
    return (CAS(&Parents[d],(intT)-1,s));
  }
  inline bool cond (intT d) { return (Parents[d] == -1); }
};

timer t1,t2;

template <class vertex>
void BFS(intT start, graph<vertex> GA) {
  
  startTime();
  intT n = GA.n;
  bool denseForward = 0;
  intT* Parents = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) Parents[i] = -1;
  Parents[start] = start;

  vertices Frontier(n,start);

  intT round = 0;
  timer t1;
  intT numVisited = 0;
  while(!Frontier.isEmpty()){
    round++;
    numVisited+=Frontier.numNonzeros();
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t1.start();
    vertices output = edgeMap(GA, Frontier, BFS_F(Parents),GA.m/20,denseForward);    
    t1.stop();
    Frontier.del();
    Frontier = output;
  } 
  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t1.reportTotal("MVMult");
  Frontier.del();
  free(Parents); 
  nextTime("Total");
  // cout<<"Vertices visited = "<<numVisited<<endl;
  // cout<<"Edges traversed = "<<edgesTraversed<<endl;
}

int parallel_main(int argc, char* argv[]) {  
  char* iFile;
  bool binary = false;
  bool symmetric = false;
  if(argc > 1) iFile = argv[1];
  if(argc > 2) if((string) argv[2] == (string) "-s") symmetric = true;
  if(argc > 3) if((string) argv[3] == (string) "-b") binary = true;

  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric,binary);
    BFS((intT)1,G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    BFS((intT)1,G);
    G.del();
  }
}
