#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "utils.h"
#include "parallel.h"

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "serialSort.h"
#endif

using namespace std;

struct edgeSecond {
  bool operator() (edge<intT> a, edge<intT> b) {
    return a.v < b.v;
  }; 
};

struct isAdj {
  intT v;
  isAdj (intT _v)
    : v(_v) {};
  bool operator() (edge<intT> e){
    return e.u == v || e.v == v;
  };
};


intT localCount(edgeArray<intT> EA, intT v, intT* degrees, intT* offsets, intT* colors, intT* properColor) { 
  intT m = EA.nonZeros;
  intT d = degrees[v];
  intT o = offsets[v];

  //edges touching v (FROM BOTH SIDES)
  edge<intT>* Eadj = newA(edge<intT>,m);
  intT m_adj = sequence::filter(EA.E,Eadj,m,isAdj(v));

  //neighbors of v
  intT* nghDegrees = newA(intT, m_adj);
  parallel_for(intT i=0;i<m_adj;i++) {
    intT ngh = (Eadj[i].u == v) ? Eadj[i].v : Eadj[i].u;
    nghDegrees[i] = degrees[ngh];
  }
  intT totalNghDegree = sequence::plusScan(nghDegrees,nghDegrees,m_adj);
  
  //edges touching neighbors of v
  edge<intT>* Eadj2 = newA(edge<intT>, totalNghDegree);
  
  parallel_for(intT i=0;i<m_adj;i++) {
    intT ngh = (Eadj[i].u == v) ? Eadj[i].v : Eadj[i].u;
    intT o = nghDegrees[i]; //offset in Eadj2
    intT nghd = ((i == m_adj-1) ? totalNghDegree : nghDegrees[i+1]) - o;
    intT oo = offsets[ngh]; //offset in original E
    for(intT j=0;j<nghd;j++) { 
      Eadj2[o+j] = EA.E[oo+j];
    }
  }
  free(nghDegrees);

  //sort distance-2 neighbors
  compSort(Eadj2,totalNghDegree,edgeSecond());

  //count triangles by scanning Eadj and Eadj2 in tandem
  intT a=0, b=0, count = 0;
  while(1) { if(Eadj2[b].v == -1 && b < totalNghDegree) b++; else break; }
  while(1) {
    if(a == m_adj || b == totalNghDegree) break;
    else if(Eadj[a].v == -1) a++;
    intT ngh = (Eadj[a].v == v) ? Eadj[a].u : Eadj[a].v;
    if(ngh == Eadj2[b].v) { 
      intT vs[3];
      vs[0] = v; vs[1] = Eadj2[b].u; vs[2] = Eadj2[b].v;
      compSort(vs,3,less<intT>());

      //only increment count if colors are right
      if(properColor[0] == colors[vs[0]] && properColor[1] == colors[vs[1]] && properColor[2] == colors[vs[2]]) count++;	 
      b++;
    }
    else if(ngh < Eadj2[b].v) a++;
    else b++;
  }

  free(Eadj); 
  free(Eadj2);
  
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

  intT* offsets = newA(intT,n);
  sequence::plusScan(degrees,offsets,n);

  intT* C = newA(intT, n+1); 
  intT threshold = (2+m)/4;

  //find high-degree vertices
  parallel_for(intT i=0;i<n;i++) {
    if(degrees[i] >= threshold) C[i] = i;
    else C[i] = -1;
  }
  intT* high = newA(intT,n);
  intT nHigh = sequence::filter(C,high,n,notNeg());
  intT count = 0;

  //do local count for high-degree vertices
  for(intT i=0;i<nHigh;i++) {
    count += localCount(EA, high[i], degrees, offsets, colors, properColor);
    parallel_for(intT j=0;j<m;j++) {
      if(E[j].u == high[i] || E[j].v == high[i])
      	{E[j].u = E[j].v = -1;}
    }
  }

  free(high);

  //remove high-degree vertices and incidence edges and recurse
  intT* labels = C;
  parallel_for(intT i=0;i<n;i++) 
    labels[i] = (degrees[i] < threshold);
  labels[n] = 0;
  intT new_n = sequence::plusScan(labels,labels,n+1);

  parallel_for(intT i=0;i<m;i++) {
    intT u = E[i].u, v = E[i].v;
    if(u == -1 || v == -1) E[i].u = -1;
    else E[i].u = labels[u]; E[i].v = labels[v];
  }

  //get edges without high-degree vertices
  edge<intT>* E_new = newA(edge<intT>,m);
  intT new_m = sequence::filter(E,E_new,m,notNegEdge());

  free(offsets);

  m = new_m; 

  //base case
  if(m < 3) {free(E_new); free(C); return count; }

  intT* colorsPacked = newA(intT,new_n);
  //assign new colors
  parallel_for(intT i=0;i<n;i++){
    if(labels[i] != labels[i+1]) {
      colorsPacked[labels[i]] = colors[i]*2 - (utils::hash(seed+i) & 1);
    }
  }
  free(C);
  n = new_n;

  //recurse
  intT countsR[8];
  intT properColors[24];
  parallel_for(intT i=0;i<8;i++){
    intT j = i*3;
    //colors of subproblem
    properColors[j] = 2*properColor[0] - ((4 & i) >> 2);
    properColors[j+1] = 2*properColor[1] - ((2 & i) >> 1);
    properColors[j+2] = 2*properColor[2] - (1 & i);

    //get subset of edges that are consistent with coloring
    edge<intT>* E_sub = newA(edge<intT>,m);
    intT sub_m = sequence::filter(E_new,E_sub,m,inSubgraph(colorsPacked,properColors+j));

    if(sub_m < 3) { countsR[i] = 0; free(E_sub); continue; }

    //compute degrees of vertices in subgraph
    intT* flags = newA(intT,sub_m);
    flags[0] = 0;

    parallel_for(intT k=1;k<sub_m;k++) 
      if(E_sub[k].u != E_sub[k-1].u) flags[k] = k; else flags[k] = -1;

    intT* flags2 = newA(intT,n+1);
    intT uniqueFlags = sequence::filter(flags,flags2,sub_m,notNeg());
    flags2[uniqueFlags] = sub_m;
    free(flags);

    //compress vertex numbers (some vertices may not be included in subgraph)
    intT* L = newA(intT,n+1);
    parallel_for(intT k=0;k<n+1;k++) L[k] = 0;
    parallel_for(intT k=0;k<sub_m;k++) {
      intT u = E_sub[k].u, v = E_sub[k].v;
      if(!L[u]) utils::CAS(&L[u],0,1);
      if(!L[v]) utils::CAS(&L[v],0,1);
    }
    intT sub_n = sequence::plusScan(L,L,n+1);
    parallel_for(intT k=0;k<sub_m;k++) {
      intT u = E_sub[k].u, v = E_sub[k].v;
      E_sub[k].u = L[u]; E_sub[k].v = L[v];
    }

    //assign colors to vertices in subgraph 
    intT* subColors = newA(intT,sub_n);
    parallel_for(intT k=0;k<n;k++) 
      { if(L[k] != L[k+1]) subColors[L[k]] = colorsPacked[k]; }


    //compute degrees for vertices in subgraph
    intT* degreesSub = L;
    parallel_for(intT k=0;k<n;k++) degreesSub[k] = 0;

    parallel_for(intT k=0;k<uniqueFlags;k++)
      degreesSub[E_sub[flags2[k]].u] = flags2[k+1]-flags2[k];

    free(flags2);

    countsR[i] = tc(edgeArray<intT>(E_sub,sub_n,sub_n,sub_m),degreesSub,subColors,properColors+j,(i+1)*(seed+n));
    free(subColors);
    free(L); 
    free(E_sub);
  }

  free(colorsPacked);  free(E_new);

  //add up counts of subproblems
  for(intT i=0;i<8;i++) count+=countsR[i];

  return count;
}



intT countTriangle(edgeArray<intT> EA, intT* degrees)
{
  intT n = max(EA.numRows,EA.numCols);
  intT* colors = newA(intT,n);
  parallel_for(intT i=0;i<n;i++) colors[i] = 1;
  intT* properColor = newA(intT,3);
  properColor[0] = properColor[1] = properColor[2] = 1;
  intT count = tc(EA, degrees, colors, properColor, 0);
  free(properColor); free(colors); 
  cout<<"number of triangles = "<<count<<endl;
  return count;
}



