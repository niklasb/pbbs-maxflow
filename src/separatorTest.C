#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <math.h>
#include "sequence.h"
#include "gettime.h"
#include "graphGen.h"
#include "cilk.h"
#include "utils.h"
#include "quickSort.h"
#include "intSort.h"
using namespace std;

int* separator(graph G);
// **************************************************************
//    MAIN
// **************************************************************

typedef pair<double,int> diPair;

struct edgeLess{
  bool operator() (diPair a, diPair b){
    return (a.second < b.second); }
};


struct addDiPair { diPair operator() (diPair a, diPair b){
  b.first=a.first+b.first;return b;
  }
};

struct minDiPair { diPair operator() (diPair a, diPair b){
  return (a.first < b.first) ? a : b;
}
};

int bestCut(graph G){
  diPair* edges = newA(diPair,2*G.m);
  int* offsets = newA(int,G.n);
  
  cilk_for(int i=0;i<G.n;i++){
    offsets[i] = G.V[i].degree<<1;
  }

  sequence::scan(offsets,offsets,G.n,utils::addF<int>(),0);

  cilk_for(int i=0;i<G.n;i++){
    int o = offsets[i];
    for(int j=0;j<G.V[i].degree;j++){
      int jj = j<<1;
      if(G.V[i].Neighbors[j] > i){
	edges[o+jj].first = 1;
	edges[o+jj+1].first = -1;
      } else {
	edges[o+jj].first = 0;
	edges[o+jj+1].first = 0;
      }
      edges[o+jj].second = i;
      edges[o+jj+1].second = G.V[i].Neighbors[j];
      
    }
  }
 
  intSort::iSort(edges,2*G.m,G.n,utils::secondF<double,int>());
  
  sequence::scan(edges,edges,2*G.m,addDiPair(),make_pair(0.0,-1));

  //get max at offset points
  diPair* cuts = newA(diPair,G.n-1);
  cilk_for(int i=1;i<G.n;i++){
    int ii=i-1;
    cuts[ii] = edges[offsets[i]];
    cuts[ii].first /= i;
    cuts[ii].first /= G.n-i;
  }
  free(offsets);
  diPair best = sequence::reduce(cuts,G.n-1,minDiPair());

  cout<<"Sparsest cut position: cut after position " << best.second
      << " Cost: " << best.first << endl;

  free(cuts); free(edges);
  return best.second;
}

// number of off diagonals for diagonal blocks of size blockSize
double sepSize(graph G, int blockSize) {
  double k = 0;
  for (int i = 0; i < G.n; i++)
    for (int j = 0; j < G.V[i].degree; j++) 
      if ((i / blockSize) != (G.V[i].Neighbors[j] / blockSize)) k+= 1;
  return k;
}

double logCost(graph G) {
  double logs = 0.0;
  for (int i = 0; i < G.n; i++) { 
    for (int j = 0; j < G.V[i].degree; j++) {
      double logcost = log(((double) abs(i - G.V[i].Neighbors[j])) + 1);
      logs += logcost;
    }
  }
  return logs/(G.m*log(2.0));
}

// Make sure the ordering is proper
void checkOrder(int* I, int n) {
  int* II = newA(int,n);
  for (int i=0; i < n; i++) II[i] = I[i];
  sort(II,II+n);
  for (int i=0; i < n; i++)
    if (II[i] != i) {
      cout << "Bad ordering at i = " << i << " val = " << II[i] << endl;
      break;
    }
  free(II);
}

void test(graph G, float weight, string str) {
  double s8 = sepSize(G, 1<<8);
  double s16 = sepSize(G, 1<<16);
  double lc = logCost(G); 
  graphCheckConsistency(G);
  //randomly reorder
  G = graphReorder(G,NULL);

  startTime();
  int* I = separator(G);
  stopTime(weight,str);

  // Check correctness
  checkOrder(I, G.n);

  // Check block separator sizes with reordering
  graph GG = graphReorder(G,I);
  graphCheckConsistency(GG);


  if (0) {
    cout << setprecision(3) << "  Off Diagonal (256) = " 
	 << (100.*s8)/GG.m << "% before, "
	 << (100.*sepSize(GG, 1<<8))/GG.m << "% after" << endl;
    cout << "  Off Diagonal (64K blocks) = " 
	 << (100.*s16)/GG.m << "% before, "
	 << (100.*sepSize(GG, 1<<16))/GG.m << "% after" << endl;
  }

  //cout << "Cut location: " << cut << " Cut cost: " << cutCost(GG,cut) << endl;

  cout << "  Quality: Log Cost = " 
       << lc << " before, " 
       << logCost(GG) << " after" << endl;

  startTime();
  bestCut(GG);
  nextTime("Sparsest cut computation time: ");

  free(I);
  GG.del();
}

void testFile(char* s) {
  cout << endl;
  cout << s+5 << " : ";
  graph G = graphFromCsrFile(s,1);
  cout << "edges = " << G.m << endl;
  test(G,.1,"Separator");
}

void testFiles () {
  testFile((char*) "data/ND/nd24k.csrb");
  testFile((char*) "data/GHS_psdef/audikw_1.csrb");
  testFile((char*) "data/Schenk/nlpkkt120.csrb");
  testFile((char*) "data/Schenk/nlpkkt80.csrb");

  testFile((char*) "data/Boeing/pwtk.csrb");
  testFile((char*) "data/Freescale/Freescale1.csrb");

  testFile((char*) "data/GHS_psdef/inline_1.csrb");
  testFile((char*) "data/GHS_psdef/ldoor.csrb");
  testFile((char*) "data/INPRO/msdoor.csrb");
  testFile((char*) "data/Koutsovasilis/F1.csrb");

  testFile((char*) "data/Oberwolfach/bone010.csrb");
  testFile((char*) "data/Rajat/rajat31.csrb");

  testFile((char*) "data/Schenk_AFE/af_shell10.csrb");
  testFile((char*) "data/Schenk_ISEI/ohne2.csrb");
  testFile((char*) "data/Schmid/thermal2.csrb");
  testFile((char*) "data/TSOPF/TSOPF_FS_b300_c3.csrb");
  testFile((char*) "data/Wissgott/parabolic_fem.csrb");
  testFile((char*) "data/Zaoui/kkt_power.csrb");
  testFile((char*) "data/vanHeukelum/cage14.csrb");
}

int cilk_main(int argc, char *argv[]) {
  int n, m, dimension;
  if (argc > 1) n = std::atoi(argv[1]); else n = 10;

  {
    graph G = graph2DMesh(n);
    test(G, .5, "Separator (2d mesh)");
  }

  {
    graph G = graphRandomWithDimension(2, 10, n);
    test(G, .5, "Separator (rand dim, d=2, m = 10*n)");
  }

  reportTime("Separator (weighted average)");

  // testFiles();
}

