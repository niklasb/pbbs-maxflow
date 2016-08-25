#include <iostream>
#include "sequence.h"
#include "graph.h"
#include "utils.h"
#include "parallel.h"
#include <math.h>

#if defined(CILK) || defined(CILKP)
#include "sampleSort.h"
#elif defined(OPENMP)
#include "stlParallelSort.h"
#else
#include "serialSort.h"
#endif

using namespace std;

#define M 1000000
#define B 4
#define c 0.25

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


intT localCount(edgeArray<intT> EA, intT v, intT* degrees, intT* offsets) { 
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
      count++;	 
      b++;
    }
    else if(ngh < Eadj2[b].v) a++;
    else b++;
  }
  free(Eadj); 
  free(Eadj2);
  return count;
}

intT lowDegreeCount(edge<intT>* E, intT* low, intT* degreesSub, intT* colors,
		    intT sub_m, intT num_low, intT n, intT i, intT j, intT k) {
  //to do

  // intT numProcessed = 0;
  // intT roundSize = c*sub_m;
  // intT count = 0;
  // intT* flags = newA(intT,n);
  // while(numProcessed < sub_m) {
  //   parallel_for(intT i=0;i<n;i++) flags[i] = 0;
  //   for(intT i=0;i<num_low;i++) {
  //     intT v = low[i];
      
  //   }
  // }
  return 1;
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
  intT* colors;
  intT i,j,k;
  inSubgraph(intT* _colors, intT _i, intT _j, intT _k) :
    colors(_colors), i(_i), j(_j), k(_k) {};
  bool operator() (edge<intT> e){
    intT u = e.u, v = e.v;
    return (colors[u] == i && (colors[v] == j || colors[v] == k)) || (colors[u] == j && colors[v] == k) ;
  };
};


intT tc(edgeArray<intT> EA, intT* degrees) {  
  edge<intT>* E = EA.E;
  intT m = EA.nonZeros;
  intT n = EA.numRows;

  intT* offsets = newA(intT,n);
  sequence::plusScan(degrees,offsets,n);

  intT* C = newA(intT, n+1); 
  intT threshold = (intT) sqrt(m*M);
  
  //find high-degree vertices
  parallel_for(intT i=0;i<n;i++) {
    if(degrees[i] >= threshold) C[i] = i;
    else C[i] = -1;
  }
  intT* high = newA(intT,n);
  intT nHigh = sequence::filter(C,high,n,notNeg());
  intT count = 0;
  
  cout << n << " " << nHigh << endl;

  //do local count for high-degree vertices (degree > \sqrt(E/M))
  for(intT i=0;i<nHigh;i++) {
    count += localCount(EA, high[i], degrees, offsets);
    parallel_for(intT j=0;j<m;j++) {
      if(E[j].u == high[i] || E[j].v == high[i])
      	{E[j].u = E[j].v = -1;}
    }
  }

  free(offsets);

  //get edges without high-degree vertices
  edge<intT>* E_new = newA(edge<intT>,m);
  intT new_m = sequence::filter(E,E_new,m,notNegEdge());
  m = new_m;

  //algorithm for low-degree vertices (degree < \sqrt(E/M))
  intT numColors = max(1,(intT) sqrt((double)m / M));

  //get low-degree vertices
  parallel_for(intT i=0;i<n;i++) {
    if(degrees[i] < threshold) C[i] = i;
    else C[i] = -1;
  }

  intT* low = high;
  intT num_low = sequence::filter(C,low,n,notNeg());

  //assign colors to vertices
  intT* colors = newA(intT, n);
  parallel_for(intT i=0;i<num_low;i++) {
      colors[low[i]] = utils::hash(i) % numColors;
  }

  intT* counts = newA(intT,numColors*numColors*numColors);
  intT numColorsSquared = numColors*numColors;

  parallel_for(intT i=0;i<numColors;i++) {
    for(intT j=0;j<numColors;j++) {
      for(intT k=0;k<numColors;k++) {
	//get subset of edges that are consistent with coloring
	edge<intT>* E_sub = newA(edge<intT>,m); //allocate beforehand?
	intT sub_m = sequence::filter(E_new,E_sub,m,inSubgraph(colors,i,j,k));

	//compute degrees of vertices in subgraph
	intT* flags = newA(intT,sub_m);
	flags[0] = 0;

	parallel_for(intT l=1;l<sub_m;l++) 
	  if(E_sub[l].u != E_sub[l-1].u) flags[l] = l; else flags[l] = -1;

	intT* flags2 = newA(intT,n+1);
	intT uniqueFlags = sequence::filter(flags,flags2,sub_m,notNeg());
	flags2[uniqueFlags] = sub_m;
	free(flags);

	//compute degrees for vertices in subgraph
	intT* degreesSub = newA(intT,n+1);
	parallel_for(intT l=0;l<n;l++) degreesSub[l] = 0;

	parallel_for(intT l=0;l<uniqueFlags;l++)
	  degreesSub[E_sub[flags2[l]].u] = flags2[l+1]-flags2[l];

	free(flags2);

	//call subroutine
	counts[i*numColorsSquared+j*numColors+k] = 
	  lowDegreeCount(E_sub,low,degreesSub,colors,sub_m,num_low,n,i,j,k);

	free(degreesSub);
	free(E_sub);
      }
    }
  }

  free(colors);  free(E_new); free(low);

  //add up counts of subproblems
  intT low_count = sequence::plusScan(counts,counts,numColors*numColors*numColors);
  free(counts);
  return low_count + count;
}



intT countTriangle(edgeArray<intT> EA, intT* degrees)
{
  intT n = max(EA.numRows,EA.numCols);
  intT count = tc(EA, degrees);
  cout<<"number of triangles = "<<count<<endl;
  return count;
}



