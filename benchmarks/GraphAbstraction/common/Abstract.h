#include "utils.h"
#include "sequence.h"
#include "graph.h"
#include "parallel.h"
#include <stdio.h>
#include <assert.h>
#include "merge.h"
#include "gettime.h"
using namespace std;

typedef pair<intT,intT> intPair;

struct matrix {
  graph<intT> GA;
  intT lastFrontierSize,lastFrontierEdgeCount;
  bool firstTimeSparse;
matrix(graph<intT> _GA, bool _firstTimeSparse=1) : GA(_GA), firstTimeSparse(_firstTimeSparse) {
    lastFrontierSize = lastFrontierEdgeCount = -1;
  }
};

struct vec {
  intT n, m;
  void* v;
  bool isDense;
vec() : n(0),m(0) {}
vec(intT _n, void* values, intT _isDense, intT _m=0) : n(_n), v(values), isDense(_isDense), m(_m) { }

  void del(){
    free(v);
  }

  intT numRows() {
    return n;
  }

  intT numNonzeros() {
    return m;
  }

  bool isEmpty() {
    return m==0;
  }

  void add(vec& b){
    if(isDense) {
      bool* dense = (bool*) v;
      if(b.isDense) {//b dense
  	bool* b_dense = (bool*) b.v;
  	parallel_for(intT i=0;i<n;i++)
  	  if(!dense[i] && b_dense[i]) dense[i] = 1;
      }
      else {//b sparse
  	intT* b_sparse = (intT*) b.v;
  	parallel_for(intT i=0;i<b.m;i++){
	  intT x = b_sparse[i];
	  if(!dense[x]) dense[x] = 1;
	}
      }
      m = sequence::reduce<intT>(0,n,utils::addF<intT>(),sequence::boolGetA<intT>(dense));
    }
    else {
      if(b.isDense) { //b dense
  	toDense();
  	add(b);
      }
      else { //b sparse TODO: parallelize merge
  	intT* a_sparse = (intT*) v;
  	intT* b_sparse = (intT*) b.v;
  	intT* result = newA(intT,min(m+b.m,n));
  	intT a_ptr = 0, b_ptr = 0, r_ptr = 0;
  	while(1){
	  if(r_ptr >= n) abort();
  	  if(a_ptr == m) {
  	    for(;b_ptr<b.m;b_ptr++) {
  	      result[r_ptr] = b_sparse[r_ptr];
  	      r_ptr++;
  	    }
  	    break;
  	  }
  	  if(b_ptr == b.m) {
  	    for(;a_ptr<m;a_ptr++) {
	      result[r_ptr] = a_sparse[r_ptr];
  	      r_ptr++;
  	    }
  	    break;
  	  }
  	  if(a_sparse[a_ptr] < b_sparse[b_ptr]) {
  	    result[r_ptr++] = a_sparse[a_ptr++];
	  }
  	  else if(a_sparse[a_ptr] > b_sparse[b_ptr]) {
  	    result[r_ptr++] = b_sparse[b_ptr++];
  	  }
  	  else {
  	    result[r_ptr++] = a_sparse[a_ptr++];
  	    b_ptr++;
  	  }
  	}
  	free(a_sparse);
  	v = result;
  	m = r_ptr;
      }
    }
  }
  void toDense(){
    if(!isDense){
      bool* dense = newA(bool,n);
      intT* sparse = (intT*) v;
      parallel_for(intT i=0;i<n;i++) dense[i] = 0;
      parallel_for(intT i=0;i<m;i++){
  	dense[sparse[i]] = 1;
      }
      free(sparse);
      v=dense;
      isDense = true;
    }
  }
  void toSparse(){
    if(isDense){
      bool* dense = (bool*) v;
      intT* flags = newA(intT,n);

      parallel_for(intT i=0;i<n;i++) flags[i] = (intT) dense[i];
      m = sequence::plusScan(flags,flags,n);
      
      intT* sparse = newA(intT,m);

      for(intT i=0;i<n-1;i++){
      	intT currFlag = flags[i];
  	if(currFlag != flags[i+1]){
	  sparse[currFlag] = i;
      	}
      }
      if(flags[n-1] == m-1){
	sparse[m-1];
      }
      free(flags);
      free(dense);
      v=sparse;
      isDense = false;
    }
  }
  void printSparse(){
    assert(!isDense);
    intT* sparse = (intT*) v;
    cout<<"sparse: ";
    for(intT i=0;i<m;i++){
      cout<<sparse[i]<<" ";
    }
    cout<<endl;
  }
  void printDense() {
    assert(isDense);
    cout<<"dense: ";
    bool* dense = (bool*) v;
    for(intT i=0;i<n;i++){
      cout<<dense[i]<<" ";
    }
    cout<<endl;
  }
};

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

struct getFrontierDegreeDense {
  vertex<intT>* G;
  bool* Visited;
  getFrontierDegreeDense(vertex<intT>* Gr, bool* _Visited) :
    G(Gr), Visited(_Visited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] ? G[i].degree : 0);}
};

struct getFrontierDegreeSparse {
  vertex<intT>* G;
  intT* frontier;
getFrontierDegreeSparse(vertex<intT>* Gr, intT* _frontier) :
  G(Gr), frontier(_frontier) {}
  intT operator() (intT i) {
    return (intT) G[frontier[i]].degree;}
};

template <class F,class T>
  vec MVmult(matrix& M, vec& Frontier, F add, T thresholdF){
  graph<intT> GA = M.GA;
  //if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  if((M.lastFrontierSize != -1 && thresholdF(M.lastFrontierSize+M.lastFrontierEdgeCount)) || (M.lastFrontierSize==-1&&M.firstTimeSparse)) { //sparse

    //cout<<" sparse\n";
    Frontier.toSparse();
    //Frontier.printSparse();
    intT frontierSize = Frontier.m;
    intT* curr_frontier = (intT*) Frontier.v;

    intT* Counts = newA(intT,numVertices);

    parallel_for(intT i=0;i<frontierSize;i++)
      Counts[i]=G[curr_frontier[i]].degree;
    intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);

    intT* frontier_edges = newA(intT,frontierEdgeCount);
  
    {parallel_for(intT i = 0; i < frontierSize; i++) {
	intT v = curr_frontier[i];
	intT o = Counts[i];
	for (intT j=0; j < G[v].degree; j++) {
	  intT ngh = G[v].Neighbors[j];
	  if(add.addAtomic(v,ngh))
	    frontier_edges[o+j] = ngh;
	  else frontier_edges[o+j] = -1;
	}
      }}
  
    free(Counts);
    intT* next_frontier = newA(intT,min(numVertices,frontierEdgeCount));
   
    // Filter out the empty slots (marked with -1)
    intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF());
    free(frontier_edges);

    intT newfrontierEdgeCount = newfrontierSize ? sequence::reduce<intT>((intT)0,newfrontierSize,utils::addF<intT>(),getFrontierDegreeSparse(G,next_frontier)) : 0;

    M.lastFrontierSize = newfrontierSize;
    M.lastFrontierEdgeCount = newfrontierEdgeCount;
    
    return vec(Frontier.n,next_frontier,0,newfrontierSize);

  }
  else { //dense
    //cout<<" dense\n";

    Frontier.toDense();

    bool* next_frontier = newA(bool,numVertices);
    parallel_for(intT i=0;i<numVertices;i++) next_frontier[i] = 0;
    parallel_for(intT i=0;i<numVertices;i++){
      if(add.cond(i)){ 
      	for(intT j=0;j<G[i].degree;j++){
      	  intT ngh = G[i].Neighbors[j]; 
      	  if(add(ngh,i)) {
      	    next_frontier[i] = 1;
      	  }
      	}
      }
    }

    intT frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,next_frontier));

    intT frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),sequence::boolGetA<intT>(next_frontier));
    
    //cout<<frontierSize<< " "<<frontierEdgeCount<<endl;


    M.lastFrontierSize = frontierSize;
    M.lastFrontierEdgeCount = frontierEdgeCount;

    return vec(numVertices,next_frontier,1,frontierSize);
  }
}


static timer t3,t4;
template <class F,class T>
  vec MVmultAny(matrix& M, vec& Frontier, F add, T thresholdF){
  
  graph<intT> GA = M.GA;
  //if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  if((M.lastFrontierSize != -1 && thresholdF(M.lastFrontierSize+M.lastFrontierEdgeCount)) || (M.lastFrontierSize==-1&&M.firstTimeSparse)) { //sparse
    t3.start();
    //cout<<" sparse\n";
    Frontier.toSparse();
    //Frontier.printSparse();
    intT frontierSize = Frontier.m;
    intT* curr_frontier = (intT*) Frontier.v;

    intT* Counts = newA(intT,numVertices);

    parallel_for(intT i=0;i<frontierSize;i++)
      Counts[i]=G[curr_frontier[i]].degree;
    intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);
    //cout<<frontierEdgeCount<<endl;
    intT* frontier_edges = newA(intT,frontierEdgeCount);

    {parallel_for(intT i = 0; i < frontierSize; i++) {
	intT v = curr_frontier[i];
	intT o = Counts[i];
	for (intT j=0; j < G[v].degree; j++) {
	  intT ngh = G[v].Neighbors[j];
	  if(add.addAtomic(v,ngh))
	    frontier_edges[o+j] = ngh;
	  else frontier_edges[o+j] = -1;

	}
      }}

    free(Counts);
    intT* next_frontier = newA(intT,min(numVertices,frontierEdgeCount));
   
    // Filter out the empty slots (marked with -1)
    intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF());
    free(frontier_edges);

    intT newfrontierEdgeCount = newfrontierSize ? sequence::reduce<intT>((intT)0,newfrontierSize,utils::addF<intT>(),getFrontierDegreeSparse(G,next_frontier)) : 0;
       
    M.lastFrontierSize = newfrontierSize;
    M.lastFrontierEdgeCount = newfrontierEdgeCount;
    
    t3.stop();
    return vec(Frontier.n,next_frontier,0,newfrontierSize);

  }
  else { //dense
    //cout<<" dense\n";
    t4.start();
    Frontier.toDense();

    bool* next_frontier = newA(bool,numVertices);
    parallel_for(intT i=0;i<numVertices;i++) next_frontier[i] = 0;
    parallel_for(intT i=0;i<numVertices;i++){
      if(add.cond(i)){
      	for(intT j=0;j<G[i].degree;j++){
      	  intT ngh = G[i].Neighbors[j];
      	  if(add(ngh,i)) {
      	    next_frontier[i] = 1;
      	    break; //only works for BFS
      	  }
      	}
      }
    }

    intT frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,next_frontier));

    intT frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),sequence::boolGetA<intT>(next_frontier));
    
    //cout<<frontierSize<< " "<<frontierEdgeCount<<endl;


    M.lastFrontierSize = frontierSize;
    M.lastFrontierEdgeCount = frontierEdgeCount;
    t4.stop();
    return vec(numVertices,next_frontier,1,frontierSize);
  }
}

template <class F>
vec SparseMVmultAny(matrix& M, vec& Frontier, F add){
  graph<intT> GA = M.GA;
  //if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  Frontier.toSparse();

  intT frontierSize = Frontier.numNonzeros();
  intT* curr_frontier = (intT*) Frontier.v;

  intT* Counts = newA(intT,numVertices);

  parallel_for(intT i=0;i<frontierSize;i++)
    Counts[i]=G[curr_frontier[i]].degree;
  intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);
  //cout<<frontierEdgeCount<<endl;
  intT* frontier_edges = newA(intT,frontierEdgeCount);


  {parallel_for(intT i = 0; i < frontierSize; i++) {
      intT v = curr_frontier[i];
      intT o = Counts[i];
      for (intT j=0; j < G[v].degree; j++) {
	intT ngh = G[v].Neighbors[j];
	if(add.addAtomic(v,ngh))
	    frontier_edges[o+j] = ngh;
	else frontier_edges[o+j] = -1;

      }
    }}

  free(Counts);
  intT* next_frontier = newA(intT,min(numVertices,frontierEdgeCount));

  // Filter out the empty slots (marked with -1)
  intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF());
  free(frontier_edges);

  return vec(Frontier.n,next_frontier,0,newfrontierSize);
}
