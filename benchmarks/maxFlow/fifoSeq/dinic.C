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

namespace dinic {
  bool bfs() {
    pfor (intT i = 0; i < n; ++i) nodes[i].h = -1;

    queue<intT> q;
    s.processSerial([&](intT i, intT vi) {
      if (!nodes[vi].e) return;
      nodes[vi].h = 1;
      q.push(vi);
    });
    while(!q.empty()) {
      Node& v = nodes[q.front()];
      q.pop();
      for_arcs(for, v, a, {
        Node& w = nodes[a.to];
        if (a.resCap && w.h == -1) {
          w.h = v.h + 1;
          q.push(a.to);
        }
      })
    }
    //cout << "distance = " << nodes[sink].h << endl;
    return nodes[sink].h >= 0;
  }

  intT *next;
  Cap dfs(intT vi, Cap cap, int dep=0) {
    //for(int _=0;_<dep;++_)cout << "  "; cout << "vi=" << vi << " cap=" << cap << endl;
    if (vi == sink) return cap;
    Node& v = nodes[vi];
    Cap used = 0;
    for (intT i = next[vi]; i < v.last() && cap > used; ++i) {
      next[vi] = i;
      Arc& a = arcs[i];
      intT wi = a.to;
      //for(int _=0;_<dep;++_)cout << "  "; cout << "  inspecting " << vi << "->" << wi << endl;
      Node& w = nodes[wi];
      if (a.resCap > 0 && w.h == v.h + 1) {
        Cap delta = dfs(wi, min(cap - used, a.resCap),dep+1);
        used += delta;
        increaseArcFlow(a, delta);
      }
    }
    return used;
  }

  void run() {
    next = new intT[n];
    while (bfs()) {
      pfor (intT i = 0; i < n; ++i)
        next[i] = nodes[i].first;
      s.processSerial([&](intT i, intT vi) {
        Node& v = nodes[vi];
        if (!v.e) return;
        Cap delta = dfs(vi, v.e);
        v.e -= delta;
        nodes[sink].e += delta;
      });
    }
    delete[] next;
  }
}
