// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include "gettime.h"
#include "MIS.h"
using namespace std;

struct color_FF {
  vertex<intT> *_G;  intT *_Flags;  intT *_Colors;  intT _c;
  color_FF(vertex<intT> *G, intT *Flags, intT *Colors, intT c) : 
    _G(G), _Flags(Flags), _Colors(Colors), _c(c) {}
  intT operator() (intT v) {
    if (_Flags[v] == 1) {_Colors[v] = _c; _Flags[v] = 2; return 0;}
    else {_Flags[v] = 0; return 1;}
  }
};

intT colorGraphR(_seq<intT> Remain, vertex<intT>* G, intT* Flags, intT* Colors, intT c, intT round, intT roundM) {
  if (Remain.n == 0 && round < roundM) return 0;
  else {
    maxIndSetR(Remain,G,Flags,round,roundM);
    _seq<intT> R = 
      sequence::filter(Remain.A, Remain.n,color_FF(G,Flags,Colors,c));
    intT m = colorGraphR(R, G, Flags, Colors, c+1, round+1, roundM);
    R.del();
    return m + 1;
  }
}

intT* graphColoring(graph<intT> G) {
  int round = 0;
  intT* _Remain = newA(intT,G.n);
  parallel_for(intT i=0;i<G.n;i++) _Remain[i] = i;
  _seq<intT> Remain(_Remain,G.n);

  intT *Colors = newA(intT,G.n);
  intT* _Flags = newArray(G.n,(intT)0);
  _seq<intT> Flags(_Flags,G.n);

  //cilk_for (int i=0; i < G.n; i++) {Flags[i] = 0; Colors[i] = -1;}
  int m = colorGraphR(Remain,G.V,Flags.A,Colors,0,round,round+100);

  //for(intT i=0;i<G.n;i++)cout<<Colors[i]<<" ";cout<<endl;
  cout << "Colors = " << m << endl;
  return Colors;
}
