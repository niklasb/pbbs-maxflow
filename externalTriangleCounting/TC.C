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
#include <stxxl.h> // STXXL header
#include "parallel.h"
#include "gettime.h"
#include "TC.h"

using namespace std;

// **************************************************************
// * using the degree heuristic to order the vertices
// **************************************************************

struct nodeLT {
  nodeLT(stxxl::vector<intT> &_V, intT _n) : V(_V), n(_n) {};

  bool operator() (const intT &a, const intT &b) const {
    return (V[a+1]-V[a]) < (V[b+1]-V[b]);
  };

  intT min_value() { return n;} //hack

  intT max_value() { return n+1;} //hack

  stxxl::vector<intT> &V;
  intT n;
};


struct intLT {

  intLT(intT _n) : n(_n) {};

  bool operator () (const intT &a, const intT &b) const { return a < b; };

  intT min_value() { return 0; }

  intT max_value() { return n; }

  intT n;

};

void rankNodes(stxxl::vector<intT> & vertices, stxxl::vector<intT>& r, stxxl::vector<intT>& o)
{
  intT n = o.size();
  for (intT i=0;i<n;i++) o[i] = i;
  if(n < 512*1024*1024/sizeof(intT)) {
    std::sort(o.begin(), o.end(), nodeLT(vertices,n)); }
  else {
    stxxl::sort(o.begin(), o.end(), nodeLT(vertices,n), 512*1024*1024);}
  for (intT i=0;i<n;i++) r[o[i]] = i;  
}

struct isFwd{
  isFwd(intT myrank, intT *r) : r_(r), me_(myrank) {};
  bool operator () (intT v) {return r_[v] > me_;};
  intT me_;
  intT *r_;
};

intT countTriangle(stxxl::vector<intT> & vertices, stxxl::vector<intT> & edges)
{
  intT n = vertices.size()-3;
  intT m = edges.size();

  stxxl::vector<intT> rank(n);
  stxxl::vector<intT> order(n);

  rankNodes(vertices, rank, order);

  cout<<"ranked nodes\n";

  // create a directed version of G and order the nodes in
  // the increasing order of rank

  stxxl::vector<intT> sz(n);

  for (intT s=0;s<n;s++) {
    intT j = 0;
    intT d = vertices[s+1]-vertices[s];
    for(intT i=0;i<d;i++) {
      if(rank[s] < rank[edges[vertices[s]+i]]){
  	edges[vertices[s]+j] = edges[vertices[s]+i];
  	j++;
      }
    }
    sz[s] = j;
    if(j < 512*1024*1024/sizeof(intT)) 
      std::sort(edges.begin()+vertices[s],edges.begin()+vertices[s]+j,intLT(n));
    else
      stxxl::sort(edges.begin()+vertices[s],edges.begin()+vertices[s]+j,intLT(n),512*1024*1024); 
  }

  cout<<"created directed graph\n";

  // start counting

  intT count = 0;
  for(intT i=0;i<n;i++) {
    for (intT bi=0;bi<sz[i];bi++) {
      intT j=edges[vertices[i]+bi];
      //assert(rank[j] > rank[i]);
      //count common
      intT k=0,l=0;
      while (k < sz[i] && l < sz[j]) { 
  	if (edges[vertices[i]+k]==edges[vertices[j]+l]) {k++; l++; count++;}
  	else if (edges[vertices[i]+k] < edges[vertices[j]+l]) k++;
  	else l++;
      }
    }
  }
 
  cout << "tri. count = " << count << endl;
  return count; 
}
