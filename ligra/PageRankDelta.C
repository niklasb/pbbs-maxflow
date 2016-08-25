#include "ligra.h"
#include "gettime.h"
#include "math.h"
using namespace std;

template <class vertex>
struct PR_F {
  vertex* V;
  double* Delta, *nghSum;
  PR_F(vertex* _V, double* _Delta, double* _nghSum) : 
    V(_V), Delta(_Delta), nghSum(_nghSum) {}
  inline bool update(intT s, intT d){
    nghSum[d] += Delta[s]/V[s].getOutDegree();
    return 1;
  }
  inline bool updateAtomic (intT s, intT d) {
    writeAdd(&nghSum[d],Delta[s]/V[s].getOutDegree());
    return 1;
  }
  inline bool cond (intT d) { return 1; }
};

struct PR_Vertex_F_FirstRound {
  double damping, addedConstant, one_over_n, epsilon2;
  double* p, *Delta, *nghSum;
  PR_Vertex_F_FirstRound(double* _p, double* _Delta, double* _nghSum, double _damping, double _one_over_n,double _epsilon2) :
    p(_p),
    damping(_damping), Delta(_Delta), nghSum(_nghSum), one_over_n(_one_over_n),
    addedConstant((1-_damping)*_one_over_n),
    epsilon2(_epsilon2) {}
  inline bool operator () (intT i) {
    Delta[i] = damping*(p[i]+nghSum[i])+addedConstant-p[i];
    p[i] += Delta[i];
    Delta[i]-=one_over_n; //subtract off delta from initialization
    return (fabs(Delta[i]) > epsilon2 * p[i]);
  }
};

struct PR_Vertex_F {
  double damping, addedConstant, epsilon2;
  double* p, *Delta, *nghSum;
  PR_Vertex_F(double* _p, double* _Delta, double* _nghSum, double _damping, double _one_over_n,double _epsilon2) :
    p(_p),
    damping(_damping), Delta(_Delta), nghSum(_nghSum), 
    addedConstant((1-_damping)*_one_over_n),
    epsilon2(_epsilon2) {}
  inline bool operator () (intT i) {
    // double old_nghSum = (p[i]-addedConstant)/damping;
    // Delta[i] = damping*(old_nghSum+nghSum[i])+addedConstant-p[i];
    Delta[i] = nghSum[i]*damping;
    if(fabs(Delta[i]) > epsilon2*p[i]) { p[i]+=Delta[i]; return 1; }
    else return 0;
    // p[i] += Delta[i];
    // return (fabs(Delta[i]) > epsilon2 * p[i]);
  }
};

struct PR_Vertex_Reset {
  double* nghSum;
  PR_Vertex_Reset(double* _nghSum) :
    nghSum(_nghSum) {}
  inline bool operator () (intT i) {
    nghSum[i] = 0.0;
    return 1;
  }
};


timer t1,t2,t0;

template <class vertex>
void PageRankDelta(graph<vertex> GA) {
  startTime();
  bool denseWrite = 1;
  const intT n = GA.n;
  const double damping = 0.85;
  const double epsilon = 0.0000001;
  const double epsilon2 = 0.01;

  double one_over_n = 1/(double)n;
  double* p = newA(double,n);
  parallel_for(intT i=0;i<n;i++) p[i] = 0.0;//one_over_n;
  double* Delta = newA(double,n);
  parallel_for(intT i=0;i<n;i++) Delta[i] = one_over_n; //initial delta propagation from each vertex
  double* nghSum = newA(double,n);
  parallel_for(intT i=0;i<n;i++) nghSum[i] = 0.0;
  bool* frontier = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) frontier[i] = 1;

  vertices Frontier(n,n,frontier);
  bool* all = newA(bool,n);
  parallel_for(intT i=0;i<n;i++) all[i] = 1;

  vertices All(n,n,all);

  intT round = 0;
  timer t1,t2;

  intT numChanged = n;
  while(round<1){
    round++;
    if(round > 1000) { cout << "too many rounds\n"; abort(); }
    t1.start();
    // cout<<"Round "<<round<<", frontier size = "<<Frontier.numNonzeros()<<endl;
    // for(intT i=0;i<min(n,10);i++)cout<<p[i]<<" ";cout<<endl;
    vertices output = edgeMap(GA, Frontier, PR_F<vertex>(GA.V,Delta,nghSum),GA.m/20,denseWrite);
    output.del();
    t1.stop();
    t2.start();
    //cout<<"nghSum:";for(intT i=0;i<min(n,10);i++)cout<<nghSum[i]<<" ";cout<<endl;
    vertices active 
      = (round == 1) ? 
      vertexFilter(All,PR_Vertex_F_FirstRound(p,Delta,nghSum,damping,one_over_n,epsilon2)) :
      vertexFilter(All,PR_Vertex_F(p,Delta,nghSum,damping,one_over_n,epsilon2));
    //for(intT i=0;i<min(n,10);i++)cout<<p[i]<<" ";cout<<endl;
    // cout<<"Delta:";for(intT i=0;i<min(n,10);i++)cout<<Delta[i]<<" ";cout<<endl;
    // cout<<"sum of p's = "<<sequence::plusReduce(p,n)<<endl;
    // cout<<"num active = "<<active.numNonzeros()<<endl;
    t2.stop();
    t0.start();
    //compute L1-norm (use nghSum as temp array)
    parallel_for(intT i=0;i<n;i++) {
      nghSum[i] = fabs(Delta[i]);
    }
    double L1_norm = sequence::plusReduce(nghSum,n);
    //cout<<"L1 norm = "<<L1_norm<<endl;
    t0.stop();

    if(L1_norm < epsilon) break;
    t0.start();
    //reset
    vertexMap(All,PR_Vertex_Reset(nghSum));
    t0.stop();
    Frontier.del();
    Frontier = active;
  }
  cout<<"Finished in "<<round<<" iterations\n";
  Frontier.del();
  free(p); free(Delta); free(nghSum);
  All.del();
  t3.reportTotal("Sparse");
  t4.reportTotal("Dense");
  t1.reportTotal("MVMult");
  t2.reportTotal("vertexFilter");
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
    PageRankDelta(G);
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary);
    PageRankDelta(G);
    G.del();
  }
}
