#include "ligra.h"
#include "gettime.h"
#include "math.h"
using namespace std;

template <class vertex>
struct PR_F {
  double* p_curr, *p_next;
  vertex* V;
  PR_F(double* _p_curr, double* _p_next, vertex* _V) : 
    p_curr(_p_curr), p_next(_p_next), V(_V) {}
  inline bool update(intT s, intT d){
    p_next[d] += p_curr[s]/V[s].getOutDegree();
    return 1;
  }
  inline bool updateAtomic (intT s, intT d) {
    writeAdd(&p_next[d],p_curr[s]/V[s].getOutDegree());
    return 1;
  }
  inline bool cond (intT d) { return 1; }
};

struct PR_Vertex_F {
  double damping;
  double addedConstant;
  double* p_curr;
  double* p_next;
  PR_Vertex_F(double* _p_curr, double* _p_next, double _damping, intT n) :
    p_curr(_p_curr), p_next(_p_next), 
    damping(_damping), addedConstant((1-_damping)*(1/(double)n)){}
  inline bool operator () (intT i) {
    p_next[i] = damping*p_next[i] + addedConstant;
    return 1;
  }
};

struct PR_Vertex_Reset {
  double* p_curr;
  PR_Vertex_Reset(double* _p_curr) :
    p_curr(_p_curr) {}
  inline bool operator () (intT i) {
    p_curr[i] = 0.0;
    return 1;
  }
};

timer t1,t2,t0;

template <class vertex>
void PageRank(graph<vertex> GA) {
  startTime();
  bool denseWrite = 1;
  const intT n = GA.n;
  const double damping = 0.85;
  const double epsilon = 0.0000001;
  
  double one_over_n = 1/(double)n;
  double* p_curr = newA(double,n);
  parallel_for(intT i=0;i<n;i++) p_curr[i] = one_over_n;
  double* p_next = newA(double,n);
  parallel_for(intT i=0;i<n;i++) p_next[i] = 0; //0 if unchanged
  bool* frontier = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) frontier[i] = 1;

  vertices Frontier(n,n,frontier);
  
  intT round = 0;
  timer t1,t2,t0;

  intT numChanged = n;
  while(round<1){
    round++;
    if(round > 1000) { cout << "too many rounds\n"; abort(); }
    t1.start();
    vertices output = edgeMap(GA, Frontier, PR_F<vertex>(p_curr,p_next,GA.V),GA.m/20,denseWrite);
    t1.stop();
    t2.start();

    vertexMap(Frontier,PR_Vertex_F(p_curr,p_next,damping,n));

    t2.stop();
    t0.start();
    //compute L1-norm between p_curr and p_next
    parallel_for(intT i=0;i<n;i++) {
      p_curr[i] = fabs(p_curr[i]-p_next[i]);
    }
    double L1_norm = sequence::plusReduce(p_curr,n);
    //cout<<"Round "<<round<<", L1 norm = "<<L1_norm<<endl;
    t0.stop();
    if(L1_norm < epsilon) break;
    t0.start();
    //reset p_curr
    vertexMap(Frontier,PR_Vertex_Reset(p_curr));
    
    swap(p_curr,p_next);
    Frontier.del(); 
    Frontier = output;
    t0.stop();
  }
  cout<<"Finished in "<<round<<" iterations\n";
  Frontier.del();
  free(p_curr); free(p_next); 

  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("vertex map");
  t0.reportTotal("compute error, reset");
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
    PageRank(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    PageRank(G);
    G.del();
  }
}
