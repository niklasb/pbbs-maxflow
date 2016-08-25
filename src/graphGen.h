#ifndef _GRAPHGEN_INCLUDED
#define _GRAPHGEN_INCLUDED

#include "graph.h"

// **************************************************************
//    SPARSE ROW MAJOR REPRESENTATION
// **************************************************************

sparseRowMajorD sparseFromCsrFile(const char* fname);
sparseRowMajorD sparseFromMtxFile(const char* fname, bool makeSymmetric);

sparseRowMajorD sparseRandomWithDimension(int dimension, int edgesPerVertex, 
					  int n);

// **************************************************************
//    EDGE ARRAY REPRESENTATION
// **************************************************************

edgeArray edgesFromMtxFile(const char* fname);
edgeArray remDuplicates(edgeArray A);

edgeArray edge2DMesh(int n);
edgeArray edge2DMeshPlanar(int n);
edgeArray edge3DMesh(int n);
edgeArray edgeRandomWithDimension(int dim, int degree, int numRows);
edgeArray edgeRmat(int n, int m, unsigned int seed, 
		   float a, float b, float c, float d);

// **************************************************************
//    WEIGHED EDGE ARRAY
// **************************************************************

wghEdgeArray wghEdge2dMesh(int nn);
wghEdgeArray wghEdgeRandom(int n, int m);
wghEdgeArray addRandWeights(edgeArray G);

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

graph graphFromMtxFile(const char* fname, bool makeSymmetric);
graph graphFromCsrFile(const char* fname, bool makeSymmetric);

graph graphRandomWithDimension(int dimension, int edgesPerVertex, int n);
graph graph2DMesh(int n);
graph graph2DMeshPlanar(int n);
graph graph3DMesh(int n);

// **************************************************************
//    CONVERSIONS AND GENERAL TOOLS
// **************************************************************

edgeArray edgesFromSparse(sparseRowMajorD M);
graph graphFromEdges(edgeArray EA, bool makeSymmetric);
sparseRowMajorD sparseFromGraph(graph G);
wghEdgeArray addRandWeights(edgeArray G);

graph graphReorder(graph Gr, int* I);
int graphCheckConsistency(graph GR);


#endif // _GRAPHGEN_INCLUDED
