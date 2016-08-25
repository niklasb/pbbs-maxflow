#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include <math.h>
#include "seq.h"
#include "intSort.h"
#include "hash.h"
#include "cilk.h"
#include "graph.h"
#include "graphGen.h"
#include "itemGen.h"

using namespace std;
using namespace dataGen;

void graph::print() {
  cout << "vertices = " << n << endl;
  cilk_for (int i=0; i < n; i++) {
    cout << "Id = " << i;
    cout << " Deg = " << V[i].degree << " neighbors = ";
    for (int j =0; j < V[i].degree; j++) cout << (V[i].Neighbors[j]) << " ";
    cout << endl;
  }
} 

// Generates an undirected graph with n vertices with approximately degree 
// neighbors per vertex.
// Edges  are distributed so they appear to come from
// a dim-dimensional space.   In particular an edge (i,j) will have
// probability roughly proportional to (1/|i-j|)^{(d+1)/d}, giving 
// separators of size about n^{(d-1)/d}.    
edgeArray edgeRandomWithDimension(int dim, int degree, int numRows) {
  int halfDegree = degree/2;
  int nonZeros = numRows*halfDegree;
  edge *E = newA(edge,nonZeros);

  cilk_for (int k=0; k < nonZeros; k++) {
    int i = k / halfDegree;
    int pow = dim+2;
    unsigned int h = k;
    int j;
    do {
      while ((((h = hash<int>(h)) % 1000003) < 500001)) pow += dim;
      j = (i + ((h = hash<int>(h)) % (((long) 1) << pow))) % numRows;
    } while (j == i);
    E[k].u = i;  E[k].v = j;
  }
  return edgeArray(E,numRows,numRows,nonZeros);
}

int loc(int n, int i1, int i2) {
  return ((i1 + n) % n)*n + (i2 + n) % n;
}

struct rMat {
  double a, ab, abc;
  int n; 
  unsigned int h;
  rMat(int _n, unsigned int _seed, double _a, double _b, double _c,double _d) {
    n = _n; a = _a; ab = _a + _b; abc = _a+_b+_c;
    h = hash<unsigned int>(_seed);
    utils::myAssert(round(10e5 * (abc+_d))==10e5,
		    "in rMat: a + b + c + d do not add to 1");
    utils::myAssert(utils::nextPower(n) == n, "in rMat: n not a power of 2");
  }

  edge rMatRec(int nn, int start, int stride) {
    if (nn==1) return edge(0,0);
    else {
      edge x = rMatRec(nn/2, start+stride, stride);
      double r = hash<double>(start);
      if (r < a) return x;
      else if (r < ab) return edge(x.u,x.v+nn/2);
      else if (r < abc) return edge(x.u+nn/2, x.v);
      else return edge(x.u+nn/2, x.v+nn/2);
    }
  }

  edge operator() (int i) {
    unsigned int start = hash<unsigned int>((2*i)*h);
    unsigned int stride = hash<unsigned int>((2*i+1)*h);
    return rMatRec(n, start, stride);
  }
};

edgeArray edgeRmat(int n, int m, unsigned int seed, 
		   float a, float b, float c, float d) {
  int nn = utils::nextPower(n);
  rMat g(nn,seed,a,b,c,d);
  edge* E = newA(edge,m);
  cilk_for (int i = 0; i < m; i++) 
    E[i] = g(i);
  return edgeArray(E,nn,nn,m);
}

edgeArray edge2DMesh(int n) {
  int dn = round(pow((float) n,1.0/2.0));
  int nn = dn*dn;
  int nonZeros = 2*nn;
  edge *E = newA(edge,nonZeros);
  cilk_for (int i=0; i < dn; i++)
    for (int j=0; j < dn; j++) {
      int l = loc(dn,i,j);
      E[2*l] = edge(l,loc(dn,i+1,j));
      E[2*l+1] = edge(l,loc(dn,i,j+1));
    }
  return edgeArray(E,nn,nn,nonZeros);
}

edgeArray edge2DMeshPlanar(int n) {
  int dn = round(pow((float) n,1.0/2.0));
  int nn = dn*dn;
  int nonZeros = 2*nn;
  edge *E = newA(edge,nonZeros);
  cilk_for (int i=0; i < dn; i++)
    for (int j=0; j < dn; j++) {
      int l = loc(dn,i,j);
      E[2*l] = edge(l,loc(dn,i+1,j));
      E[2*l+1] = edge(l,loc(dn,i,j+1));
    }

  int finger = 0;
  for(int i=0;i<nonZeros;i++){
    if(E[i].u < E[i].v) E[finger++] = E[i];
  }
  return edgeArray(E,nn,nn,finger);
}

int loc3d(int n, int i1, int i2, int i3) {
  return ((i1 + n) % n)*n*n + ((i2 + n) % n)*n + (i3 + n) % n;
}

edgeArray edge3DMesh(int n) {
  int dn = round(pow((float) n,1.0/3.0));
  int nn = dn*dn*dn;
  int nonZeros = 3*nn;
  edge *E = newA(edge,nonZeros);
  cilk_for (int i=0; i < dn; i++)
    for (int j=0; j < dn; j++) 
      for (int k=0; k < dn; k++) {
	int l = loc3d(dn,i,j,k);
	E[3*l] =   edge(l,loc3d(dn,i+1,j,k));
	E[3*l+1] = edge(l,loc3d(dn,i,j+1,k));
	E[3*l+2] = edge(l,loc3d(dn,i,j,k+1));
      }
  return edgeArray(E,nn,nn,nonZeros);
}

wghEdgeArray addRandWeights(edgeArray G) {
  int m = G.nonZeros;
  int n = G.numRows;
  wghEdge *E = newA(wghEdge, m);
  for (int i=0; i < m; i++) {
    E[i].u = G.E[i].u;
    E[i].v = G.E[i].v;
    E[i].weight = hash<int>(i);
  }
  return wghEdgeArray(E, n, m);
}

wghEdgeArray wghEdgeRandom(int n, int m) {
  wghEdge *E = newA(wghEdge, m);

  for (int i=0; i < m; i++) {
    E[i].u = n-hash<int>(i)%n-1;
    E[i].v = n-hash<int>(i+m)%n-1;
    E[i].weight = hash<int>(i+2*m);
  }
  return wghEdgeArray(E, n, m);
}

wghEdgeArray wghEdge2dMesh(int nn) { 
  edgeArray E = edge2DMesh(nn);
  wghEdgeArray EW = addRandWeights(E);
  E.del();
  return EW;
}

sparseRowMajorD sparseFromCsrFile(const char* fname) {
  FILE *f = fopen(fname,"r");
  if (f == NULL) {
    cout << "Trying to open nonexistant file: " << fname << endl;
    abort();
  }

  int numRows;  int numCols;  int nonZeros;
  int nc = fread(&numRows, sizeof(int), 1, f);
  nc = fread(&numCols, sizeof(int), 1, f);
  nc = fread(&nonZeros, sizeof(int), 1, f); 

  double *Values = (double *) malloc(sizeof(double)*nonZeros);
  int *ColIds = (int *) malloc(sizeof(int)*nonZeros);
  int *Starts = (int *) malloc(sizeof(int)*(1 + numRows));
  Starts[numRows] = nonZeros;

  size_t r;
  r = fread(Values, sizeof(double), nonZeros, f);
  r = fread(ColIds, sizeof(int), nonZeros, f);
  r = fread(Starts, sizeof(int), numRows, f); 
  fclose(f);
  return sparseRowMajorD(numRows,numCols,nonZeros,Starts,ColIds,Values);
}

edgeArray edgesFromSparse(sparseRowMajorD M) {
  edge *E = newA(edge,M.nonZeros);
  int k = 0;
  for (int i=0; i < M.numRows; i++) {
    for (int j=M.Starts[i]; j < M.Starts[i+1]; j++) {
      if (M.Values[j] != 0.0) {
	E[k].u = i;
	E[k].v = M.ColIds[j];
	k++;
      }
    }
  }
  int nonZeros = k;
  return edgeArray(E,M.numRows,M.numCols,nonZeros);
}

edgeArray edgesFromMtxFile(const char* fname) {
  ifstream file (fname, ios::in);
  char* line = newA(char,1000);
  int i,j = 0;
  while (file.peek() == '%') {
    j++;
    file.getline(line,1000);
  }
  int numRows, numCols, nonZeros;
  file >> numRows >> numCols >> nonZeros;
  //cout << j << "," << numRows << "," << numCols << "," << nonZeros << endl;
  edge *E = newA(edge,nonZeros);
  double toss;
  for (i=0, j=0; i < nonZeros; i++) {
    file >> E[j].u >> E[j].v >> toss;
    E[j].u--;
    E[j].v--;
    if (toss != 0.0) j++;
  }
  nonZeros = j;
  //cout << "nonzeros = " << nonZeros << endl;
  file.close();  
  return edgeArray(E,numRows,numCols,nonZeros);
}

struct hashEdgeF {
  unsigned int operator() (edge *e) {
    return hash<int>(e->u) + hash<int>(2*e->v); }
};

struct cmpEdgeF {
  int operator() (edge *a, edge *b) {
    int c = intCmp()(a->u,b->u);
    return (c == 0) ? intCmp()(a->v,b->v) : c;
  }
};

seq<edge*> removeDuplicates(seq<edge*> S, int m) {
  return removeDuplicates(S,m,cmpEdgeF(),hashEdgeF(), (edge*) NULL);}

edgeArray remDuplicates(edgeArray A) {
  int m = A.nonZeros;
  edge **EP = newA(edge*,m);
  cilk_for (int i=0;i < m; i++) EP[i] = A.E+i;
  seq<edge*> F = removeDuplicates(seq<edge*>(EP,m), m);
  free(EP);
  int l = F.sz;
  edge *E = newA(edge,m);
  cilk_for (int i=0; i < l; i++) E[i] = *F.S[i];
  F.del();
  return edgeArray(E,A.numRows,A.numCols,l);
}

struct nEQF {bool operator() (edge e) {return (e.u != e.v);}};

edgeArray makeSymmetric(edgeArray A) {
  int m = A.nonZeros;
  edge *E = A.E;
  edge *F = newA(edge,2*m);
  int mm = sequence::filter(E,F,m,nEQF());
  cilk_for (int i=0; i < mm; i++) {
    F[i+mm].u = F[i].v;
    F[i+mm].v = F[i].u;
  }
  edgeArray R = remDuplicates(edgeArray(F,A.numRows,A.numCols,2*mm));
  free(F);
  return R;
}

struct getuF {vindex operator() (edge e) {return e.u;} };

graph graphFromEdges(edgeArray EA, bool makeSym) {
  edgeArray A;
  if (makeSym) A = makeSymmetric(EA);
  else {  // should have copy constructor
    edge *E = newA(edge,EA.nonZeros);
    cilk_for (int i=0; i < EA.nonZeros; i++) E[i] = EA.E[i];
    A = edgeArray(E,EA.numRows,EA.numCols,EA.nonZeros);
  }
  int m = A.nonZeros;
  int n = A.numRows;
  int* offsets = newA(int,n*2);
  intSort::iSort(A.E,offsets,m,n,getuF());
  int *X = newA(int,m);
  vertex *v = newA(vertex,n);
  cilk_for (int i=0; i < n; i++) {
    int o = offsets[i];
    int l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].degree = l;
    v[i].Neighbors = X+o;
    for (int j=0; j < l; j++) {
      v[i].Neighbors[j] = A.E[o+j].v;
    }
  }
  A.del();
  free(offsets);
  return graph(v,n,m,X);
}

sparseRowMajorD sparseFromGraph(graph G) {
  int numRows = G.n;
  int nonZeros = G.m;
  vertex* V = G.V;
  int *Starts = newA(int,numRows+1);
  int *ColIds = newA(int,nonZeros);
  int start = 0;
  for (int i = 0; i < numRows; i++) {
    Starts[i] = start;
    start += V[i].degree;
  }
  Starts[numRows] = start;
  cilk_for (int j=0; j < numRows; j++)
    for (int i = 0; i < (Starts[j+1] - Starts[j]); i++) {
      ColIds[Starts[j]+i] = V[j].Neighbors[i];
    }
  return sparseRowMajorD(numRows,numRows,nonZeros,Starts,ColIds,NULL);
}

graph graphFromMtxFile(const char* fname, bool makeSymmetric) {
  edgeArray EA = edgesFromMtxFile(fname);
  graph G = graphFromEdges(EA, makeSymmetric);
  EA.del();
  return G;
}

sparseRowMajorD sparseFromMtxFile(const char* fname, bool makeSymmetric) {
  graph G = graphFromMtxFile(fname,makeSymmetric);
  sparseRowMajorD M = sparseFromGraph(G);
  G.del();
  return M;
}

graph graphFromCsrFile(const char* fname, bool makeSymmetric) {
  sparseRowMajorD M = sparseFromCsrFile(fname);
  edgeArray EA = edgesFromSparse(M);
  M.del();
  graph G = graphFromEdges(EA, makeSymmetric);
  EA.del();
  return  G;
}

graph graphRandomWithDimension(int dimension, int edgesPerVertex, int n) {
  edgeArray EA = edgeRandomWithDimension(dimension, edgesPerVertex, n);
  graph G = graphFromEdges(EA, 1);
  EA.del();
  return G;
}

graph graph2DMesh(int n) {
  edgeArray EA = edge2DMesh(n);
  graph G = graphFromEdges(EA, 1);
  EA.del();
  return G;
}

graph graph2DMeshPlanar(int n) {
  edgeArray EA = edge2DMeshPlanar(n);
  graph G = graphFromEdges(EA, 1);
  EA.del();
  return G;
}

graph graph3DMesh(int n) {
  edgeArray EA = edge3DMesh(n);
  graph G = graphFromEdges(EA, 1);
  EA.del();
  return G;
}

sparseRowMajorD sparseRandomWithDimension(int dimension, int edgesPerVertex, int n) {
  graph G = graphRandomWithDimension(dimension, edgesPerVertex, n);
  sparseRowMajorD M = sparseFromGraph(G);
  G.del();
  return M;
}

// if I is NULL then it randomly reorders
graph graphReorder(graph Gr, int* I) {
  int n = Gr.n;
  int m = Gr.m;
  bool noI = (I==NULL);
  if (noI) {
    I = newA(int,Gr.n);
    cilk_for (int i=0; i < Gr.n; i++) I[i] = i;
    random_shuffle(I,I+Gr.n);
  }
  vertex *V = newA(vertex,Gr.n);
  for (int i=0; i < Gr.n; i++) V[I[i]] = Gr.V[i];
  for (int i=0; i < Gr.n; i++) {
    for (int j=0; j < V[i].degree; j++) {
      V[i].Neighbors[j] = I[V[i].Neighbors[j]];
    }
    sort(V[i].Neighbors,V[i].Neighbors+V[i].degree);
  } 
  free(Gr.V);
  if (noI) free(I);
  return graph(V,n,m,Gr.allocatedInplace);
}

int graphCheckConsistency(graph Gr) {
  vertex *V = Gr.V;
  int edgecount = 0;
  for (int i=0; i < Gr.n; i++) {
    edgecount += V[i].degree;
    for (int j=0; j < V[i].degree; j++) {
      vindex ngh = V[i].Neighbors[j];
      utils::myAssert(ngh >= 0 && ngh < Gr.n,
		      "graphCheckConsistency: bad edge");
    }
  }
  if (Gr.m != edgecount) {
    cout << "bad edge count in graphCheckConsistency: m = " 
	 << Gr.m << " sum of degrees = " << edgecount << endl;
    abort();
  }
  return 0;
}
