#include "ligra.h"
#include "gettime.h"
using namespace std;

template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

struct Radii_F {
  intT round;
  intT* radii;
  long* Visited, *NextVisited;
  Radii_F(long* _Visited, long* _NextVisited, intT* _radii, intT _round) : 
    Visited(_Visited), NextVisited(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool update (intT s, intT d){
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      NextVisited[d] |= toWrite;
      if(radii[d] != round) { radii[d] = round; return 1; }
    }
    return 0;
  }
  inline bool updateAtomic (intT s, intT d){
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      writeOr(&NextVisited[d],toWrite);
      intT oldRadii = radii[d];
      if(radii[d] != round) return CAS(&radii[d],oldRadii,round);
    }
    return 0;
  }
  inline bool cond (intT d) { return 1; }
};

struct Radii_Vertex_F {
  long* Visited, *NextVisited;
  Radii_Vertex_F(long* _Visited, long* _NextVisited) :
    Visited(_Visited), NextVisited(_NextVisited) {}
  inline bool operator() (intT i) {
    NextVisited[i] |= Visited[i];
    return 1;
  }
};

timer t1,t2,t5;

template <class vertex>
void Radii(graph<vertex> GA) {
  startTime();
  bool denseForward = 0;
  intT n = GA.n;

  intT* radii = newA(intT,n);
  long* Visited = newA(long,n);
  long* NextVisited = newA(long,n);
  parallel_for(intT i=0;i<n;i++) {
    radii[i] = -1;
    Visited[i] = NextVisited[i] = 0;
  }
  intT sampleSize = min<intT>(n,64);
  intT* starts = newA(intT,sampleSize);
  parallel_for(intT i=0;i<sampleSize;i++) { 
    radii[i] = 0;
    starts[i] = i; 
    Visited[i] = (long) 1<<i;
  }
  vertices Frontier(n,sampleSize,starts);

  intT round = 0;
  timer t1;

  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t5.start();
    vertexMap(Frontier, Radii_Vertex_F(Visited,NextVisited));
    t5.stop();
    t1.start();
    vertices output = edgeMap(GA, Frontier, Radii_F(Visited,NextVisited,radii,round),GA.m/20,denseForward);
    t1.stop();
    swap(NextVisited,Visited);
    Frontier.del();
    Frontier = output;
  }
  free(Visited); free(NextVisited);
  
  t3.reportTotal("sparse");
  t4.reportTotal("dense");
  t5.reportTotal("copy");
  t1.reportTotal("MVMult");
  Frontier.del();
  
  //radiiCheck(GA,radii);

  free(radii); 
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
    Radii(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    Radii(G);
    G.del();
  }
}
