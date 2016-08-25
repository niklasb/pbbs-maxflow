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

// Adds a random integer weight to each node and edge 

#include <math.h>
#include <iostream>
#include "IO.h"
#include "graph.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "dataGen.h"
#include "parallel.h"
using namespace benchIO;
using namespace dataGen;
using namespace std;

typedef long long ll;
typedef wghVertex<intT> V;
ll rand_range(pair<ll,ll> range) {
  return (((ll)rand() << 16) | rand()) % (range.second - range.first + 1) + range.first;
}

const char nl = '\n';

int main(int argc, char* argv[]) {
  srand(time(NULL));
  commandLine P(argc,argv,"<inFile> <outFile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;
  ofstream out(oFile, ofstream::binary);

  wghGraph<intT> g = readWghGraphFromFile<intT>(iFile);
  intT n = g.n, m = g.m;
  intT maxW = utils::log2Up(n);
  intT *supply = new intT[n];
  for (intT i = 0; i < n; ++i) {
    supply[i] = rand_range(make_pair(-maxW, maxW));
    if (supply[i]) m++;
  }
  intT S = n + 1, T = n + 2;

  out << "c DIMACS flow network description" << nl;
  out << "c (problem-id, nodes, arcs)" << nl;
  out << "p max " << n + 2 << " " << m << nl;

  out << "c source" << nl;
  out << "n " << S << " s" << nl;
  out << "c sink" << nl;
  out << "n " << T << " t" << nl;

  out << "c arc description (from, to, capacity)" << nl;

  for (intT i = 0; i < n; ++i) {
    V& v = g.V[i];
    for (intT j = 0; j < v.degree; ++j) {
      out << "a " << i + 1 << " " << v.Neighbors[j] + 1 << " "
          << v.nghWeights[j] << nl;
    }
  }

  for (intT i = 0; i < n; ++i) {
    if (supply[i] > 0)
      out << "a " << S << " " << i + 1 << " " << supply[i] << nl;
    else if (supply[i] < 0)
      out << "a " << i + 1 << " " << T << " " << -supply[i] << nl;
  }
}
