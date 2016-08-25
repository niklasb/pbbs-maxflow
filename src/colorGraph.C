#include <iostream>
#include <cstdlib>
#include "seq.h"
#include "graph.h"
#include "gettime.h"
#include "cilk.h"
using namespace std;

void maxIndSetR(seq<vindex> Remain, vertex* G, int* Flags, 
		int round, int maxRound);

struct color_FF {
  vertex *_G;  int *_Flags;  int *_Colors;  int _c;
  color_FF(vertex *G, int *Flags, int *Colors, int c) : 
    _G(G), _Flags(Flags), _Colors(Colors), _c(c) {}
  int operator() (vindex v) {
    if (_Flags[v] == 1) {_Colors[v] = _c; _Flags[v] = 2; return 0;}
    else {_Flags[v] = 0; return 1;}
  }
};

int colorGraphR(seq<vindex> Remain, vertex* G, int* Flags, int* Colors, int c, int round, int roundM) {
  if (Remain.size() == 0 && round < roundM) return 0;
  else {
    maxIndSetR(Remain,G,Flags,round,roundM);
    seq<vindex> R = Remain.filter(color_FF(G,Flags,Colors,c));
    int m = colorGraphR(R, G, Flags, Colors, c+1, round+1, roundM);
    R.del();
    return m + 1;
  }
}

int* colorGraph(graph G, int seed) {
  int round = seed;
  seq<vindex> Remain = seq<vindex>(G.n, utils::identityF<int>());
  int *F = newA(int,G.n);
  int *Colors = newA(int,G.n);
  seq<int> Flags = seq<int>(G.n, utils::zeroF<int>());
  //cilk_for (int i=0; i < G.n; i++) {Flags[i] = 0; Colors[i] = -1;}
  int m = colorGraphR(Remain,G.V,Flags.S,Colors,0,round,round+100);
  //cout << "Colors = " << m << endl;
  return Colors;
}
