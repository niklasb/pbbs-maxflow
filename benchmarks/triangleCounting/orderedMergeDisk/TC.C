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
#include <cassert>
//#include "sequence.h"
#include "graph.h"
//#include "utils.h"
//#include "parallel.h"
#include <stxxl.h> // STXXL header
#include <stxxl/vector>

using namespace std;

// achieve an ascending order of sorting
struct my_comparator
{
bool operator()(const int &a, const int &b) const
{
return a < b;
}
int min_value() const {
return std::numeric_limits<int>::min();
}
int max_value() const {
return std::numeric_limits<int>::max();
}
};


// **************************************************************
// * using the degree heuristic to order the vertices
// **************************************************************

struct nodeLT {
  nodeLT(graph<intT> G) : G_(G) {};

  bool operator() (const intT &a, const intT &b) const { 
    return G_.V[a].degree < G_.V[b].degree;
  };

  intT min_value() { return 0; }

  intT max_value() { return G_.n; }

  graph<intT> G_;
};

void rankNodes(graph<intT> G, stxxl::vector<intT>& r, stxxl::vector<intT>& o)
{
  for (intT i=0;i<G.n;i++) o[i] = i;
  stxxl::sort(o.begin(), o.end(), nodeLT(G), 512*1024*1024);
  for (intT i=0;i<G.n;i++) r[o[i]] = i;  
}

struct isFwd{
  isFwd(intT myrank, intT *r) : r_(r), me_(myrank) {};
  bool operator () (intT v) {return r_[v] > me_;};
  intT me_;
  intT *r_;
};

struct intLT {

  intLT(graph<intT> G) : G_(G) {};

  bool operator () (const intT &a, const intT &b) const { return a < b; };

  intT min_value() { return 0; }

  intT max_value() { return G_.n; }

  graph<intT> G_;

};

intT countTriangle(graph<intT> G)
{

  stxxl::vector<intT> rank(G.n);
  stxxl::vector<intT> order(G.n);
  rankNodes(G, rank, order);

  // create a directed version of G and order the nodes in
  // the increasing order of rank

  stxxl::vector<intT> edges(G.m);
  stxxl::vector<intT> sz(G.n);
  stxxl::vector<intT> start(G.n);
  
  // compute starting point in edges
  for (intT i=0;i<G.n;i++) sz[i] = G.V[i].degree;
  for (intT i=1;i<G.n;i++) start[i] = start[i-1]+sz[i-1];
  
  for (intT s=0;s<G.n;s++) {
    intT j = 0;
    for(intT i=0;i<G.V[s].degree;i++) {
      if(rank[s] < rank[G.V[s].Neighbors[i]]) {
	edges[start[s]+j] = G.V[s].Neighbors[i];
	j++;
      }
    }
    sz[s] = j;
  
    stxxl::sort(edges.begin()+start[s],edges.begin()+start[s]+sz[s],intLT(G),1024*1024);
  }

  // start counting

  intT count = 0;

  for(intT i=0;i<G.n;i++) {
    for (intT bi=0;bi<sz[i];bi++) {
      intT j=edges[start[i]+bi];
      //assert(rank[j] > rank[i]);
   
      //count common
      intT k=0,l=0;   
      while (k < sz[i] && l < sz[j]) {
	if (edges[start[i]+k]==edges[start[j]+l]) k++, l++, count++;
	else if (edges[start[i]+k] < edges[start[j]+l]) k++;
	else l++;
      }
    }
  }
  

  cout << "tri. count = " << count << endl;
  return count; 
}
