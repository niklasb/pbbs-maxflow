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

template <class E>
struct matrix {
  graph<intT> GA;
  intT* mVisited;
  E* mValues;
  intT lastFrontierSize,lastFrontierEdgeCount;
  E zero;
  bool firstTimeSparse;
matrix(graph<intT> _GA, E _zero, bool _firstTimeSparse=1) : GA(_GA), zero(_zero), firstTimeSparse(_firstTimeSparse) {
    mVisited = newA(intT,_GA.n);
    mValues = newA(E,_GA.n);
    parallel_for(intT i=0;i<GA.n;i++) {
      mVisited[i] = 0;
      mValues[i] = zero;
    }
    lastFrontierSize = lastFrontierEdgeCount = -1;
  }
  void del() {
    free(mVisited);
    free(mValues);
  }
};

template <class E>
struct vec {
  typedef pair<E,intT> E_intPair;
  intT n, m;
  bool (*isZero)(E);
  E zero;
  void* v;
  bool isDense;
vec() : n(0),m(0) {}
vec(intT _n, void* values, intT _isDense, bool (*_isZero)(E), E _zero, intT _m=0) : n(_n), v(values), isDense(_isDense), m(_m), isZero(_isZero), zero(_zero) { }

  void del(){
    free(v);
  }

  template <class F>
  void add(vec<E>& b, F f){
    if(isDense) {
      E* dense = (E*) v;
      if(b.isDense) {//b dense
	E* b_dense = (E*) b.v;
	parallel_for(intT i=0;i<n;i++)
	  dense[i] = f(dense[i],b_dense[i]);
      }
      else {//b sparse
	E_intPair* b_sparse = (E_intPair*) b.v;
	parallel_for(intT i=0;i<b.m;i++)
	  dense[b_sparse[i].second] = f(dense[b_sparse[i].second],b_sparse[i].first);
      }
    } 
    else {
      if(b.isDense) { //b dense
	toDense();
	add(b,f);
      }
      else { //b sparse TODO: parallelize merge
	E_intPair* a_sparse = (E_intPair*) v;
	E_intPair* b_sparse = (E_intPair*) b.v;
	E_intPair* result = newA(E_intPair,min(m+b.m,n));
	intT a_ptr = 0, b_ptr = 0, r_ptr = 0;
	while(1){
      
	  if(r_ptr >= n) abort();
	  if(a_ptr == m) {
	    for(;b_ptr<b.m;b_ptr++) {
	      result[r_ptr] = make_pair(b_sparse[b_ptr].first,r_ptr);
	      r_ptr++;
	    }
	    break;
	  }
	  if(b_ptr == b.m) {
	    for(;a_ptr<m;a_ptr++) {
	      result[r_ptr] = make_pair(a_sparse[a_ptr].first,r_ptr);
	      r_ptr++;
	    }
	    break;
	  }
	  if(a_sparse[a_ptr].second < b_sparse[b_ptr].second) {
	    result[r_ptr] = make_pair(a_sparse[a_ptr++].first,r_ptr);
	    r_ptr++;
	  }
	  else if(a_sparse[a_ptr].second > b_sparse[b_ptr].second) {
	    result[r_ptr] = make_pair(b_sparse[b_ptr++].first,r_ptr);
	    r_ptr++;
	  }
	  else {
	    result[r_ptr] = make_pair(f(a_sparse[a_ptr++].first,b_sparse[b_ptr++].first),r_ptr);
	    r_ptr++;
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
      E* dense = newA(E,n);
      E_intPair* sparse = (E_intPair*) v;
      parallel_for(intT i=0;i<n;i++) dense[i] = zero;
      parallel_for(intT i=0;i<m;i++){
	dense[sparse[i].second] = sparse[i].first;
      }
      free(sparse);
      v=dense;
      isDense = true;
    }
  }
  void toSparse(){
    if(isDense){
      E* dense = (E*) v;
      intT* flags = newA(intT,n);

      parallel_for(intT i=0;i<n;i++) flags[i] = isZero(dense[i]) ? 0 : 1;
      m = sequence::plusScan(flags,flags,n);
      
      E_intPair* sparse = newA(E_intPair,m);

      for(intT i=0;i<n-1;i++){
      	intT currFlag = flags[i];
	if(currFlag != flags[i+1]){
      	  sparse[currFlag].first = dense[i];
      	  sparse[currFlag].second = i;
      	}
      }
      if(flags[n-1] == m-1){
      	sparse[m-1].first = dense[n-1];
      	sparse[m-1].second = n-1;
      }
      free(flags);
      free(dense);
      v=sparse;
      isDense = false;      
    }
  }
  void printSparse(){
    assert(!isDense);
    pair<E,intT>* sparse = (pair<E,intT>*) v;  
    for(intT i=0;i<m;i++){
      cout<<"("<<sparse[i].first<<","<<sparse[i].second<<")";
    }
    cout<<endl;
  }
  void printDense() {
    assert(isDense);
    E* dense = (E*) v;
    for(intT i=0;i<n;i++){
      cout<<dense[i]<<" ";
    }
    cout<<endl;
  }
};

struct nonNegF{bool operator() (intT a) {return (a>=0);}};

template <class E>
struct nonNegF2nd{bool operator() (pair<E,intT> a) {return (a.second>=0);}};

struct getFrontierSize {
  vertex<intT>* G;
  intT* Visited, *NextVisited;
  getFrontierSize(vertex<intT>* Gr, intT* _Visited, intT* _NextVisited) :
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? 1 : 0);}
};

struct getFrontierDegreeDense {
  vertex<intT>* G;
  intT* Visited, *NextVisited;
  getFrontierDegreeDense(vertex<intT>* Gr, intT* _Visited, intT* _NextVisited) :
    G(Gr), Visited(_Visited), NextVisited(_NextVisited) {}
  intT operator() (intT i) {
    return (intT) (Visited[i] != NextVisited[i] ? G[i].degree : 0);}
};

template <class E>
struct getFrontierDegreeSparse {
  vertex<intT>* G;
  pair<E,intT>* frontier;
getFrontierDegreeSparse(vertex<intT>* Gr, pair<E,intT>* _frontier) :
  G(Gr), frontier(_frontier) {}
  intT operator() (intT i) {
    return (intT) G[frontier[i].second].degree;}
};

template <class E,class F,class H,class T>
  vec<E> MVmult(matrix<E>& M, vec<E>& Frontier, F add, 
		H mult, T thresholdF, intT*& Visited){
  typedef pair<E,intT> E_intPair;
  graph<intT> GA = M.GA;
  if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  if((M.lastFrontierSize != -1 && thresholdF(M.lastFrontierSize+M.lastFrontierEdgeCount)) || (M.lastFrontierSize==-1&&M.firstTimeSparse)) { //sparse
    cout<<" sparse\n";
    Frontier.toSparse();
    
    //Frontier.printSparse();
    intT frontierSize = Frontier.m;
    pair<E,intT>* curr_frontier = (pair<E,intT>*) Frontier.v;

    intT* Counts = newA(intT,numVertices);

    parallel_for(intT i=0;i<frontierSize;i++)
      Counts[i]=G[curr_frontier[i].second].degree;
    intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);
    //cout<<frontierEdgeCount<<endl;
    pair<E,intT>* frontier_edges = newA(E_intPair,frontierEdgeCount);
    
    intT* mVisited = M.mVisited;
    E* mValues = M.mValues;
    //cout<<"mValues: ";for(intT i=0;i<numVertices;i++)cout<<mValues[i]<<" ";cout<<endl;

    {parallel_for(intT i = 0; i < frontierSize; i++) {
    	intT v = curr_frontier[i].second;
    	intT o = Counts[i];
    	for (intT j=0; j < G[v].degree; j++) {
    	  intT ngh = G[v].Neighbors[j];
    	  if (Visited[ngh] == 0) {
	      if(utils::CAS(&mVisited[ngh],(intT)0,(intT)1))
		{
		  frontier_edges[o+j].second = ngh;
		  //frontier_edges[o+j].first = v;
		}
	      else frontier_edges[o+j].second = -1;
	      add.addAtomic(mValues[ngh],mult(curr_frontier[i].first,v));
	  }
    	  else frontier_edges[o+j].second = -1;
	  
    	  //if(Visited[ngh] == 0) Add.addAtomic(Vector[ngh],Vector[v]);
    	}
      }}
   
  
    free(Counts);
    pair<E,intT>* next_frontier = newA(E_intPair,min(numVertices,frontierEdgeCount));
   
    // Filter out the empty slots (marked with -1)
    intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF2nd<E>());
    free(frontier_edges);

    intT newfrontierEdgeCount = newfrontierSize ? sequence::reduce<intT>((intT)0,newfrontierSize,utils::addF<intT>(),getFrontierDegreeSparse<E>(G,next_frontier)) : 0;

    parallel_for(intT i=0;i<newfrontierSize;i++){
      Visited[next_frontier[i].second] = 1;
      mVisited[next_frontier[i].second] = 0;
      next_frontier[i].first = mValues[next_frontier[i].second];
      mValues[next_frontier[i].second] = M.zero;
    }

    M.lastFrontierSize = newfrontierSize;
    M.lastFrontierEdgeCount = newfrontierEdgeCount;

    return vec<E>(Frontier.n,next_frontier,0,Frontier.isZero,Frontier.zero,newfrontierSize);

  }
  else { //dense
    cout<<" dense\n";
    Frontier.toDense();
    E* r_dense = newA(E,numVertices);
    E* v_dense = (E*) Frontier.v;
    parallel_for(intT i=0;i<numVertices;i++)r_dense[i]=Frontier.zero;

    intT* mVisited = M.mVisited;
    parallel_for(intT i=0;i<numVertices;i++){

      mVisited[i] = Visited[i];
      if(!mVisited[i]){
    	for(intT j=0;j<G[i].degree;j++){
    	  intT ngh = G[i].Neighbors[j];
    	  if(Visited[ngh]) {
	    r_dense[i] = add(r_dense[i],mult(v_dense[ngh],ngh));
    	    //add([i],Vector[ngh]);
    	    mVisited[i] = 1;
    	    //break; //only works for BFS
    	  }	  
    	}
      }
    }

    intT frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,mVisited));
    
    intT frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierSize(G,Visited,mVisited));
    
    //cout<<frontierSize<< " "<<frontierEdgeCount<<endl;

    parallel_for(intT i=0;i<numVertices;i++)
      if(mVisited[i]) { Visited[i] = 1; mVisited[i] = 0; }

    M.lastFrontierSize = frontierSize;
    M.lastFrontierEdgeCount = frontierEdgeCount;

    return vec<E>(Frontier.n,r_dense,1,Frontier.isZero,Frontier.zero,frontierSize);    
  }
}


template <class E,class F,class H,class T>
  vec<E> MVmultOR(matrix<E>& M, vec<E>& Frontier, F add, 
		  H mult, T thresholdF, intT*& Visited){
  typedef pair<E,intT> E_intPair;
  graph<intT> GA = M.GA;
  if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  if((M.lastFrontierSize != -1 && thresholdF(M.lastFrontierSize+M.lastFrontierEdgeCount)) || (M.lastFrontierSize==-1&&M.firstTimeSparse)) { //sparse
    //cout<<" sparse\n";
    Frontier.toSparse();
    //Frontier.printSparse();
    intT frontierSize = Frontier.m;
    pair<E,intT>* curr_frontier = (pair<E,intT>*) Frontier.v;

    intT* Counts = newA(intT,numVertices);

    parallel_for(intT i=0;i<frontierSize;i++)
      Counts[i]=G[curr_frontier[i].second].degree;
    intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);
    //cout<<frontierEdgeCount<<endl;
    pair<E,intT>* frontier_edges = newA(E_intPair,frontierEdgeCount);
    
    
    intT* mVisited = M.mVisited;
    E* mValues = M.mValues;

    {parallel_for(intT i = 0; i < frontierSize; i++) {
    	intT v = curr_frontier[i].second;
    	intT o = Counts[i];
    	for (intT j=0; j < G[v].degree; j++) {
    	  intT ngh = G[v].Neighbors[j];
    	  if (Visited[ngh] == 0 && utils::CAS(&mVisited[ngh],(intT)0,(intT)1))
    	    {
    	      frontier_edges[o+j].second = ngh;
	      //frontier_edges[o+j].first = v;
	      add.addAtomic(mValues[ngh],mult(curr_frontier[i].first,v));
	      //add.addAtomic(mValues[ngh],v);
    	    }
    	  else frontier_edges[o+j].second = -1;
    	  //if(Visited[ngh] == 0) Add.addAtomic(Vector[ngh],Vector[v]);
    	}
      }}
    free(Counts);
    E_intPair* next_frontier = newA(E_intPair,min(numVertices,frontierEdgeCount));
   
    // Filter out the empty slots (marked with -1)
    intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF2nd<E>());
    free(frontier_edges);

    intT newfrontierEdgeCount = newfrontierSize ? sequence::reduce<intT>((intT)0,newfrontierSize,utils::addF<intT>(),getFrontierDegreeSparse<E>(G,next_frontier)) : 0;
   
    parallel_for(intT i=0;i<newfrontierSize;i++){
      Visited[next_frontier[i].second] = 1;
      mVisited[next_frontier[i].second] = 0;
      next_frontier[i].first = mValues[next_frontier[i].second];
      mValues[next_frontier[i].second] = M.zero;
    }
    
    M.lastFrontierSize = newfrontierSize;
    M.lastFrontierEdgeCount = newfrontierEdgeCount;
    
    return vec<E>(Frontier.n,next_frontier,0,Frontier.isZero,Frontier.zero,newfrontierSize);

  }
  else { //dense
    //cout<<" dense\n";
    Frontier.toDense();
    E* r_dense = newA(E,numVertices);
    E* v_dense = (E*) Frontier.v;
    parallel_for(intT i=0;i<numVertices;i++)r_dense[i]=Frontier.zero;

    intT* mVisited = M.mVisited;
    parallel_for(intT i=0;i<numVertices;i++){
      mVisited[i] = Visited[i];
      if(!mVisited[i]){
    	for(intT j=0;j<G[i].degree;j++){
	  
    	  intT ngh = G[i].Neighbors[j];
	  
    	  if(Visited[ngh]) {
	    r_dense[i] = add(r_dense[i],mult(v_dense[ngh],ngh));
	    //r_dense[i] = add(v_dense[i],ngh);
	    mVisited[i] = 1;
    	    break; //only works for BFS
    	  }
    	}
      }
    }

    intT frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,mVisited));
      
    intT frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierSize(G,Visited,mVisited));

    //cout<<frontierSize<< " "<<frontierEdgeCount<<endl;

    parallel_for(intT i=0;i<numVertices;i++)
      if(mVisited[i]) { Visited[i] = 1; mVisited[i] = 0; }

    M.lastFrontierSize = frontierSize;
    M.lastFrontierEdgeCount = frontierEdgeCount;

    return vec<E>(Frontier.n,r_dense,1,Frontier.isZero,Frontier.zero,frontierSize);    
  }
}

static timer t3,t4,t5,t6,t7;
template <class E,class F,class H,class T>
  vec<E> SparseMVmultOR(matrix<E>& M, vec<E>& Frontier, F add, 
			H mult, T thresholdF, intT*& Visited){
  typedef pair<E,intT> E_intPair;
  graph<intT> GA = M.GA;
  //if(GA.n != Frontier.n) abort();
  intT numVertices = GA.n;
  intT numEdges = GA.m;
  vertex<intT> *G = GA.V;

  Frontier.toSparse();
  //Frontier.printSparse();
  intT frontierSize = Frontier.m;
  pair<E,intT>* curr_frontier = (pair<E,intT>*) Frontier.v;

  intT* Counts = newA(intT,numVertices);
  t6.start();
  parallel_for(intT i=0;i<frontierSize;i++)
    Counts[i]=G[curr_frontier[i].second].degree;
  intT frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize);
  //cout<<frontierEdgeCount<<endl;
  pair<E,intT>* frontier_edges = newA(E_intPair,frontierEdgeCount);
  t6.stop();  
  intT* mVisited = M.mVisited;
  E* mValues = M.mValues;
  t3.start();
  {parallel_for(intT i = 0; i < frontierSize; i++) {
      intT v = curr_frontier[i].second;
      intT o = Counts[i];
      for (intT j=0; j < G[v].degree; j++) {
	intT ngh = G[v].Neighbors[j];
	if (Visited[ngh] == 0 && utils::CAS(&mVisited[ngh],(intT)0,(intT)1))
	  {
	    frontier_edges[o+j].second = ngh;
	    //frontier_edges[o+j].first = v;
	    add.addAtomic(mValues[ngh],v);
	  }
	else frontier_edges[o+j].second = -1;
	//if(Visited[ngh] == 0) Add.addAtomic(Vector[ngh],Vector[v]);
      }
    }}
  t3.stop();
  free(Counts);
  pair<E,intT>* next_frontier = newA(E_intPair,min(numVertices,frontierEdgeCount));
  t4.start();
  // Filter out the empty slots (marked with -1)
  intT newfrontierSize = sequence::filter(frontier_edges,next_frontier,frontierEdgeCount,nonNegF2nd<E>());
  free(frontier_edges);
  t4.stop();
  t5.start();
  parallel_for(intT i=0;i<newfrontierSize;i++){
    Visited[next_frontier[i].second] = 1;
    mVisited[next_frontier[i].second] = 0;
    next_frontier[i].first = mValues[next_frontier[i].second];
    mValues[next_frontier[i].second] = M.zero;
  }
  t5.stop();
  return vec<E>(Frontier.n,next_frontier,0,Frontier.isZero,Frontier.zero,newfrontierSize);
}
