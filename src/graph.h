#ifndef _GRAPH_INCLUDED
#define _GRAPH_INCLUDED

typedef int vindex;

// **************************************************************
//    SPARSE ROW MAJOR REPRESENTATION
// **************************************************************

template <class ETYPE>
struct sparseRowMajor {
  int numRows;
  int numCols;
  int nonZeros;
  int* Starts;
  int* ColIds;
  ETYPE* Values;
  void del() {free(Starts); free(ColIds); if (Values != NULL) free(Values);}
  sparseRowMajor(int n, int m, int nz, int* S, int* C, ETYPE* V) :
    numRows(n), numCols(m), nonZeros(nz), 
    Starts(S), ColIds(C), Values(V) {}
};

typedef sparseRowMajor<double> sparseRowMajorD;

// **************************************************************
//    EDGE ARRAY REPRESENTATION
// **************************************************************

struct edge {
  vindex u;
  vindex v;
  edge(vindex f, vindex s) : u(f), v(s) {}
};

struct edgeArray {
  edge* E;
  int numRows;
  int numCols;
  int nonZeros;
  void del() {free(E);}
  edgeArray(edge *EE, int r, int c, int nz) :
    E(EE), numRows(r), numCols(c), nonZeros(nz) {}
  edgeArray() {}
};

// **************************************************************
//    WEIGHED EDGE ARRAY
// **************************************************************

struct wghEdge {
  vindex u, v;
  int weight;
  wghEdge() {}
  wghEdge(vindex _u, vindex _v, int w) : u(_u), v(_v), weight(w) {}
};

struct wghEdgeArray {
  wghEdge *E;
  int n; int m;
  wghEdgeArray(wghEdge* EE, int nn, int mm) : E(EE), n(nn), m(mm) {}
  void del() { free(E);}
};

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

struct vertex {
  vindex* Neighbors;
  int degree;
  void del() {free(Neighbors);}
  vertex(vindex* N, int d) : Neighbors(N), degree(d) {}
};

struct graph {
  vertex *V;
  int n;
  int m;
  vindex* allocatedInplace;
  graph(vertex* VV, int nn, int mm) 
    : V(VV), n(nn), m(mm), allocatedInplace(NULL) {}
  graph(vertex* VV, int nn, int mm, vindex* ai) 
    : V(VV), n(nn), m(mm), allocatedInplace(ai) {}
  void print();
  void del() {
    if (allocatedInplace == NULL) 
      for (int i=0; i < n; i++) V[i].del();
    else free(allocatedInplace);
    free(V);
  }
};

#endif // _GRAPH_INCLUDED
