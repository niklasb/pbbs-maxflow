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

#include <cstring>
#include <limits>
#include "graph.h"
#include "utils.h"
#include "parallel.h"
#include "maxFlow.h"
#include "gettime.h"
using namespace std;

template<typename intT>
struct Dinic {
  typedef intT captype;
  static const intT flowlimit = numeric_limits<intT>::max();
  intT n, m, S, T, top, *g, *next, *q, *d;
  struct edge {
    intT v, next;
    captype c;
  } *e;

  Dinic(intT n, intT m, intT S, intT T)
    : n(n), m(m), S(S), T(T), top(0)
    , g(new intT[n])
    , next(new intT[n])
    , q(new intT[n])
    , d(new intT[n])
    , e(new edge[2 * m])
  {
    memset(g, -1, sizeof(*g) * n);
  }

  ~Dinic() {
    delete[] g;
    delete[] next;
    delete[] q;
    delete[] d;
    delete[] e;
  }

  inline intT addedge(intT u, intT v, captype c) {
    e[top] = (edge) { v, g[u], c };
    g[u] = top++;
    e[top] = (edge) { u, g[v], 0 };
    g[v] = top++;
    return top - 2;
  }

  bool bfs() {
    intT qh = 0, qt = 1;
    memset(d, -1, sizeof(*d) * n);
    d[q[0] = S] = 0;
    while (qh < qt) {
      intT v = q[qh++];
      for (intT p = g[v]; p != -1; p = e[p].next)
        if (e[p].c > 0 && d[e[p].v] == -1)
          d[e[p].v] = d[v] + 1, q[qt++] = e[p].v;
    }
    return d[T] >= 0;
  }

  captype dfs(intT v, captype cap) {
    if (v == T)  return cap;
    captype used = 0;
    for (intT p = next[v]; p != -1 && cap > used; p = e[p].next) {
      next[v] = p;
      if (e[p].c > 0 && d[e[p].v] == d[v] + 1) {
        captype inc = dfs(e[p].v, min(cap - used, e[p].c));
        used += inc;
        e[p].c -= inc;
        e[p ^ 1].c += inc;
      }
    }
    return used;
  }

  captype computeMaxFlow() {
    captype maxflow = 0;
    while (bfs()) {
      memcpy(next, g, sizeof(g[0]) * n);
      maxflow += dfs(S, flowlimit);
    }
    return maxflow;
  }
};

intT maxFlow(FlowGraph<intT>& g) {
  intT n = g.g.n, m = g.g.m;
  Dinic<intT> d(n, m, g.source, g.sink);
  for (intT i = 0; i < n; ++i) {
    FlowVertex& v = g.g.V[i];
    for (intT j = 0; j < v.degree; ++j) {
      intT cap = v.nghWeights[j];
      v.nghWeights[j] = d.addedge(i, v.Neighbors[j], cap);
    }
  }
  beforeHook();
  intT flow = d.computeMaxFlow();
  afterHook();
  for (intT i = 0; i < n; ++i) {
    FlowVertex& v = g.g.V[i];
    for (intT j = 0; j < v.degree; ++j)
      v.nghWeights[j] = d.e[v.nghWeights[j] ^ 1].c;
  }
  cout << "flow=" << flow << endl;
  return flow;
}
