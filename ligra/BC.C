#include "ligra.h"
#include <vector>
#include "math.h"
#include "parallel.h"
using namespace std;

typedef double fType;

struct BC_F {
  fType* NumPaths;
  bool* Visited;

  BC_F(fType* _NumPaths, bool* _Visited) : 
    NumPaths(_NumPaths), Visited(_Visited) {}
  inline bool update(intT s, intT d){
    fType oldV = NumPaths[d];
    NumPaths[d] += NumPaths[s];
    return oldV == 0.0;
  }
  inline bool updateAtomic (intT s, intT d) {
    volatile fType oldV, newV; 
    do {
      oldV = NumPaths[d]; newV = oldV + NumPaths[s];
    } while(!CAS(&NumPaths[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; }
};

struct BC_Back_F {
  fType* Dependencies;
  bool* Visited;
  BC_Back_F(fType* _Dependencies, bool* _Visited) : 
    Dependencies(_Dependencies), Visited(_Visited) {}
  inline bool update(intT s, intT d){
    fType oldV = Dependencies[d];
    Dependencies[d] += Dependencies[s];
    //Dependencies[d] += ((NumPaths[d]/NumPaths[s])*(1+Dependencies[s]));
    return oldV == 0.0;
  }
  inline bool updateAtomic (intT s, intT d) {
    volatile fType oldV, newV;
    do {
      oldV = Dependencies[d];
      //newV = oldV + (NumPaths[d]/NumPaths[s])*(1+Dependencies[s]);
      newV = oldV + Dependencies[s];
    } while(!CAS(&Dependencies[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; }
};

struct BC_Vertex_F {
  bool* Visited;
  BC_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    return 1;
  }
};

struct BC_Back_Vertex_F {
  bool* Visited;
  fType* Dependencies, *inverseNumPaths;
  BC_Back_Vertex_F(bool* _Visited, fType* _Dependencies, fType* _inverseNumPaths) : 
    Visited(_Visited), Dependencies(_Dependencies), inverseNumPaths(_inverseNumPaths) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    Dependencies[i] += inverseNumPaths[i];
    return 1;
  }
};

timer t1,t2,t0;

template <class vertex>
void BC(intT start, graph<vertex> GA) {
  startTime();
  bool denseForward = 0;
  intT n = GA.n;
  intT threshold = GA.m/20;

  fType* NumPaths = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) NumPaths[i] = 0.0;
  NumPaths[start] = 1.0;

  bool* Visited = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;
  Visited[start] = 1;
  vertices Frontier(n,start);
 
  vector<vertices> Levels;
  Levels.push_back(Frontier);

  intT round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    t1.start();
    vertices output = edgeMap(GA, Frontier, BC_F(NumPaths,Visited),threshold,denseForward);
    t1.stop();
    t2.start();
    vertexMap(output, BC_Vertex_F(Visited));
    t2.stop();
    Levels.push_back(output);
    Frontier = output;
  }
  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("Merging output");

  //BCCheck(start,GA,NumPaths);
 
  t0.start();
  fType* Dependencies = newA(fType,n);
  parallel_for(intT i=0;i<n;i++) Dependencies[i] = 0.0;

  //invert numpaths
  fType* inverseNumPaths = NumPaths;
  parallel_for(intT i=0;i<n;i++) inverseNumPaths[i] = 1/inverseNumPaths[i];

  Levels[round].del();
  //reuse Visited
  parallel_for(intT i=0;i<n;i++) Visited[i] = 0;
  Frontier = Levels[round-1];
  vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));

  //for(intT i=0;i<n;i++)cout<<NumPaths[i]<<" ";cout<<endl;
  for(intT r=round-2;r>=0;r--) {
    //cout<<"Round "<<r<<" "<<Frontier.numNonzeros()<<endl;
    vertices output = edgeMap(GA, Frontier, 
			      BC_Back_F(Dependencies,Visited),threshold,denseForward);
    output.del();
    Frontier.del();
    Frontier = Levels[r];
    vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));
  }
  
  Frontier.del();
  t0.reportTotal("Backwards phase");

  //for(intT i=0;i<n;i++) cout<<Dependencies[i]<<" ";cout<<endl;

  parallel_for(intT i=0;i<n;i++) {
    Dependencies[i]=(Dependencies[i]-inverseNumPaths[i])/inverseNumPaths[i];
  }
  free(inverseNumPaths);

  free(Visited);
  //BCCheck2(start,GA,Dependencies);
  free(Dependencies);
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
    BC((intT)1,G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    BC((intT)1,G);
    G.del();
  }
}
