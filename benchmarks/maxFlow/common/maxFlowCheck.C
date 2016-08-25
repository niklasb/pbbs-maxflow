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
#include <algorithm>
#include <cstring>
#include <vector>
#include <queue>
#include "parallel.h"
#include "IO.h"
#include "graph.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "maxFlow.h"
using namespace std;
using namespace benchIO;

bool findResidualPath(FlowGraph<intT>& g, FlowGraph<intT>& flow, intT from, intT to) {
  vector<bool> vis(g.g.n, 0);
  queue<intT> q;
  vis[from] = 1;
  q.push(from);
  while (!q.empty()) {
    intT x = q.front(); q.pop();
    FlowVertex& vg = g.g.V[x];
    FlowVertex& vf = flow.g.V[x];
    for (intT i = 0; i < vg.degree; ++i) {
      intT y = vg.Neighbors[i];
      intT cap = vg.nghWeights[i];
      intT f = vf.nghWeights[i];
      if (f == cap) continue;
      if (y == to) return 1;
      if (!vis[y]) {
        vis[y] = 1;
        q.push(y);
      }
    }
  }
  return vis[to];
}

int checkMaxFlow(FlowGraph<intT> g, FlowGraph<intT> flow) {
  intT n = g.g.n, m = g.g.m;
  intT S = g.source, T = g.sink;
  bool valid = 1;
  intT* bal = newA(intT, n);
  memset(bal, 0, n * sizeof *bal);
  for (intT i = 0; i < n; ++i) {
    FlowVertex& vg = g.g.V[i];
    FlowVertex& vf = flow.g.V[i];
    for (intT j = 0; j < vg.degree; ++j) {
      intT ngh = vg.Neighbors[j];
      intT f = vf.nghWeights[j];
      intT cap = vg.nghWeights[j];
      if (f < 0 || f > cap) {
        cout << "Capacity constraint not satisfied for edge ("
             << i << ", " << ngh << ", " << cap << "). Flow: " << f << endl;
        valid = 0;
      }
      bal[i] -= f;
      bal[ngh] += f;
    }
  }
  if (!valid) return 1;
  parallel_for (intT i = 0; i < S; ++i) {
    if (bal[i] != 0)  {
      cout << "Flow balance constraint not satisfied for node " << i << ". Balance: "
           << bal[i] << " (should be 0)" << endl;
      valid = 0;
    }
  }
  if (!valid) return 1;
  if (bal[S] > 0) {
    cout << "Source has positive balance" << endl;
    return 1;
  }
  if (bal[T] < 0) {
    cout << "Sink has negative balance" << endl;
    return 1;
  }
  if (bal[S] + bal[T] != 0) {
    cout << "Balances of source and sink don't match"  << endl;
    return 1;
  }
  if (findResidualPath(g, flow, S, T)) {
    cout << "There is an augmenting path, so the flow is not maximal" << endl;
    return 1;
  }
  return 0;
}


int main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<inFile> <outfile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;

  ifstream in(iFile, ifstream::binary);
  ifstream in2(oFile, ifstream::binary);
  FlowGraph<intT> g = readFlowGraph<intT>(in);
  FlowGraph<intT> flow = readFlowGraph<intT>(in2);
  return checkMaxFlow(g, flow);
}
