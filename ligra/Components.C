#include "ligra.h"
#include "gettime.h"
using namespace std;

struct CC_F {
  intT* IDs;
  intT* prevIDs;
  CC_F(intT* _IDs, intT* _prevIDs) : 
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool update(intT s, intT d){
    intT origID = IDs[d];
    if(IDs[s] < origID) {
      IDs[d] = min(origID,IDs[s]);
      if(origID == prevIDs[d]) return 1;
    }
    return 0;
  }
  inline bool updateAtomic (intT s, intT d) {
    intT origID = IDs[d];
    return (writeMin(&IDs[d],IDs[s]) 
	    && origID == prevIDs[d]);
  }
  inline bool cond (intT d) { return 1; }
};

struct CC_Vertex_F {
  intT* IDs;
  intT* prevIDs;
  CC_Vertex_F(intT* _IDs, intT* _prevIDs) :
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator () (intT i) {
    prevIDs[i] = IDs[i];
    return 1;
  }
};

timer t1,t2,t0;

template <class vertex>
void Components(graph<vertex> GA) {
  startTime();
  bool denseForward = 0;
  intT n = GA.n;

  intT* IDs = newA(intT,n);
  intT* prevIDs = newA(intT,n);

  parallel_for(intT i=0;i<n;i++) IDs[i] = i;

  bool* frontier = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) frontier[i] = 1;

  vertices Frontier(n,n,frontier);
 
  intT round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t2.start();
    vertexMap(Frontier,CC_Vertex_F(IDs,prevIDs));
    t2.stop();
    t1.start();
    vertices output = edgeMap(GA, Frontier, CC_F(IDs,prevIDs),GA.m/20,denseForward);
    t1.stop();
    Frontier.del();
    Frontier = output;
  }
  Frontier.del();


  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("Copy");

  free(IDs); free(prevIDs);
  nextTime("Total");
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
    Components(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    Components(G);
    G.del();
  }
}
