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
#include "utils.h"
#include "parallel.h"
#include "graphUtils.h"

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "serialSort.h"
#endif

using namespace std;

// **************************************************************
// * using the degree heuristic to order the vertices
// **************************************************************

struct edgeLexico {
  bool operator() (edge<intT> a, edge<intT> b) {
    return (a.u == b.u) ? (a.v < b.v) : a.u < b.u;
  }; 
};

struct edgeFirst {
  bool operator() (edge<intT> a, edge<intT> b) {
    return a.u < b.u;
  }; 
};

struct edgeSecond {
  bool operator() (edge<intT> a, edge<intT> b) {
    return a.v < b.v;
  }; 
};

struct isAdj {
  intT v;
  edge<intT>* E;
  isAdj (intT _v, edge<intT>* _E)
    : v(_v), E(_E) {};

  bool operator() (edge<intT> e){
    return e.u == v || e.v == v;
  };
};


intT localCount(edgeArray<intT> EA, intT v, intT* degrees, intT* offsets, intT* colors, intT* properColor) {
  //cout<<"localCount\n"<<flush; 
  intT m = EA.nonZeros;
  intT d = degrees[v];
 
  edge<intT>* Eadj = newA(edge<intT>, d);
  intT o = offsets[v];
  //edges touching v
  parallel_for(intT i=0;i<d;i++) 
    if(colors[EA.E[o+i].v] == properColor[1]) Eadj[i] = EA.E[o+i]; 
    else Eadj[i].u = -1;
  intT* nghDegrees = newA(intT, d);
  parallel_for(intT i=0;i<d;i++) if(Eadj[i].u != -1) nghDegrees[i] = degrees[Eadj[i].v]; else nghDegrees[i] = 0;
  intT totalNghDegree = sequence::plusScan(nghDegrees,nghDegrees,d);
  
  //edges touching neighbor of v
  edge<intT>* Eadj2 = newA(edge<intT>, totalNghDegree);
  
  parallel_for(intT i=0;i<d;i++) {
    if(Eadj[i].u != -1) {
      intT ngh = Eadj[i].v;
      intT o = nghDegrees[i]; //offset in Eadj2
      intT nghd = ((i == d-1) ? totalNghDegree : nghDegrees[i+1]) - o;
      intT oo = offsets[ngh]; //offset in original E
      for(intT j=0;j<nghd;j++) {
	if(colors[EA.E[oo+j].v] == properColor[2]) Eadj2[o+j] = EA.E[oo+j];
	else Eadj2[o+j].v = -1;
      }
    }
  }
  free(nghDegrees);
  
  //sort distance-2 neighbors
  compSort(Eadj2,totalNghDegree,edgeSecond());
  
  //count triangles by scanning Eadj and Eadj2 in tandem
  intT a=0, b=0, count = 0;
  while(1) { if(Eadj2[b].v == -1) b++; else break; }
  while(1) {
    if(a == d || b == totalNghDegree) break;
    else if(Eadj[a].u == -1) a++;
    else if(Eadj[a].u == Eadj2[b].v) { count++; a++; b++; }
    else if(Eadj[a].u < Eadj2[b].v) a++;
    else b++;
  }
  
  free(Eadj); free(Eadj2);
  
  return count;
}

struct notNegEdge {
  bool operator() (edge<intT> e){
    return e.u != -1;
  };
};

struct notNeg {
  bool operator() (intT i){
    return i != -1;
  };
};

struct inSubgraph {
  intT* colorsPacked;
  intT* properColors;
  inSubgraph(intT* _colorsPacked, intT* _properColors) :
    colorsPacked(_colorsPacked), properColors(_properColors) {};

  bool operator() (edge<intT> e){
    intT u = e.u, v = e.v;
    return (colorsPacked[u] == properColors[0] && (colorsPacked[v] == properColors[1] || colorsPacked[v] == properColors[2])) || (colorsPacked[u] == properColors[1] && colorsPacked[v] == properColors[2]) ;
  };
};


intT tc(edgeArray<intT> EA, intT* degrees, intT* colors, intT* properColor, intT seed) {
  edge<intT>* E = EA.E;
  intT m = EA.nonZeros;
  intT n = EA.numRows;
  //cout<<"m="<<m<<" n="<<n<<endl;
  if(m < 3) return 0; //base case
  compSort(E, m, edgeLexico());
  // for(intT i=0;i<m;i++) cout <<"("<<E[i].u<<","<<E[i].v<<") ";cout<<endl;
  // for(intT i=0;i<n;i++) cout << degrees[i] <<" ";cout<<endl;

  intT* offsets = newA(intT,n);
  sequence::plusScan(degrees,offsets,n);

  intT* C = newA(intT, n+1); 
  intT threshold = m/4;

  //do local count for high-degree vertices
  parallel_for(intT i=0;i<n;i++) {
    C[i] = 0;
    if(degrees[i] >= threshold && properColor[0] == colors[i])
      C[i] = localCount(EA, i, degrees, offsets, colors, properColor);
  }
  intT count = sequence::plusReduce(C,n);

  //remove high-degree vertices and incidence edges and recurse
  intT* labels = C;
  parallel_for(intT i=0;i<n;i++) labels[i] = degrees[i] < threshold;
  labels[n] = 0;
  intT new_n = sequence::plusScan(labels,labels,n+1);
  parallel_for(intT i=0;i<m;i++) {
    intT u = E[i].u, v = E[i].v;
    if(degrees[u] >= threshold || degrees[v] >= threshold) E[i].u = -1;
    else E[i].u = labels[u]; E[i].v = labels[v];
  }

  edge<intT>* E_new = newA(edge<intT>,m);
  intT new_m = sequence::filter(E,E_new,m,notNegEdge());

  //freed on the way up from recursion
  //free(E);

  // EA.numRows = EA.numCols = new_n;
  // EA.nonZeros = new_m;
  // EA.E = E_new;
  free(offsets);
  // cout<<n<<" "<<m<<endl;
  // cout<<new_n<<" "<<new_m<<endl;

  m = new_m; 

  intT* colorsPacked = newA(intT,new_n);
  //reverse mapping and assign new colors
  intT* labelsUp = newA(intT,new_n);
  parallel_for(intT i=0;i<n;i++)
    if(labels[i] != labels[i+1]) {
      labelsUp[labels[i]] = i;
      colorsPacked[labels[i]] = colors[i]*2 - (utils::hash(seed+i) & 1);
    }
  free(C);
  n = new_n;
  
  //recurse
  intT* countsR = newA(intT,8);
  intT* properColors = newA(intT,24);
  parallel_for(intT i=0;i<8;i++){
    //cout<<"subproblem "<<i<<endl;
    if(m < 3) continue;
    intT j = i*3;
    properColors[j] = 2*properColor[0] - ((4 & i) >> 2);
    properColors[j+1] = 2*properColor[1] - ((2 & i) >> 1);
    properColors[j+2] = 2*properColor[2] - (1 & i);
    //get subset of edges
    edge<intT>* E_sub = newA(edge<intT>,m);
    intT sub_m = sequence::filter(E_new,E_sub,m,inSubgraph(colorsPacked,properColors+j));
    //cout<<"m="<<m<<" sub_m="<<sub_m<<endl;
    if(sub_m < 3) { free(E_sub); continue; }
    //cout<<"colors:";for(intT k=0;k<n;k++) cout<<colorsPacked[k]<<" ";cout<<endl; 
    //cout<<"c0="<<properColors[j]<<" c1="<<properColors[j+1]<<" c2="<<properColors[j+2]<<endl;

    //cout<<"E_sub: ";for(intT k=0;k<sub_m;k++)cout<<"("<<E_sub[k].u<<","<<E_sub[k].v<<") ";cout<<endl;
    //compute degrees
    intT* flags = newA(intT,sub_m);
    flags[0] = 0;
    parallel_for(intT k=1;k<sub_m;k++) 
      if(E_sub[k].u != E_sub[k-1].u) flags[k] = k; else flags[k] = -1;
    //cout<<"flags: ";for(intT k=0;k<sub_m;k++)cout<<flags[k]<<" ";cout<<endl;
    intT* flags2 = newA(intT,n+1);
    intT uniqueFlags = sequence::filter(flags,flags2,sub_m,notNeg());
    flags2[uniqueFlags] = sub_m;
    //cout<<"flags2: ";for(intT k=0;k<uniqueFlags+1;k++)cout<<flags2[k]<<" ";cout<<endl;

    free(flags);
    intT* degreesSub = newA(intT,n);
    parallel_for(intT k=0;k<n;k++) degreesSub[k] = 0;
    parallel_for(intT k=0;k<uniqueFlags;k++)
      degreesSub[E_sub[flags2[k]].u] = flags2[k+1]-flags2[k];
    
    free(flags2);
    // cout<<"degreesSub: ";
    // for(intT k=0;k<n;k++)cout<<degreesSub[k]<<" ";cout<<endl;
    // cout<<"seed = "<<seed<<endl;
    countsR[i] = tc(edgeArray<intT>(E_sub,n,n,sub_m),degreesSub,colorsPacked,properColors+j,(i+1)*(seed+n));
 
    free(degreesSub); 
    free(E_sub);
 
  }
    
  for(intT i=0;i<8;i++) count+=countsR[i];
  free(countsR); free(properColors); free(colorsPacked);

  return count;
}

timer t;

struct directedEdge {
  intT* degrees;
  directedEdge (intT* _degrees)
    : degrees(_degrees) {};

  bool operator() (edge<intT> e){
    return (degrees[e.u] == degrees[e.v]) ? e.u < e.v : degrees[e.u] < degrees[e.v];
  };
};


intT countTriangle(graph<intT> G)
{
  intT* degrees = newA(intT,G.n);
  parallel_for(intT i=0;i<G.n;i++) degrees[i] = G.V[i].degree;
  edgeArray<intT> EA = edgesFromGraph(G);
  //create directed version
  edge<intT>* E = newA(edge<intT>, EA.nonZeros);
  intT m = sequence::filter(EA.E,E,EA.nonZeros,directedEdge(degrees));
  EA.E = E; EA.nonZeros = m;
  parallel_for(intT i=0;i<G.n;i++){
    intT d=0;
    for(intT j=0;j<G.V[i].degree;j++) {
      intT ngh = G.V[i].Neighbors[j]; 
      if (G.V[ngh].degree > G.V[i].degree) d++;
    }
    degrees[i] = d;
  }

  // for(intT i=0;i<20;i++)cout<<degrees[i]<<" ";cout<<endl;
  // for(intT i=0;i<100;i++)cout<<"("<<E[i].u<<","<<E[i].v<<") ";cout<<endl;
  t.start();
  intT* colors = newA(intT,G.n);
  parallel_for(intT i=0;i<G.n;i++) colors[i] = 1;
  intT* properColor = newA(intT,3);
  properColor[0] = properColor[1] = properColor[2] = 1;
  intT count = tc(EA, degrees, colors, properColor, 0);
  free(properColor); free(colors); EA.del();
  t.reportTotal("triangle counting");
  return count;
}



