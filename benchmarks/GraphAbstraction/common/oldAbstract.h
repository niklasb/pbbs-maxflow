

/* template <class E, class F, class TF, class SF> */
/*   void MV(graph<intT> GA, intT* Frontier, intT initFrontierSize, E* Vector, F Add, TF thresholdF, SF stoppingF ){ */


/*   intT numVertices = GA.n; */
/*   intT numEdges = GA.m; */
/*   vertex<intT> *G = GA.V; */
/*   intT* NextVisited = newA(intT,numVertices); */
/*   intT* Visited = newA(intT,numVertices); */
/*   intT* FrontierNext = newA(intT,numEdges); */
/*   intT* Counts = newA(intT,numVertices); */


/*   {parallel_for(intT i = 0; i < numVertices; i++) { */
/*       Visited[i] = 0; */
/*       NextVisited[i] = 0; */
/*       Counts[i] = 0; */
/*     } */
/*   } */
/*   intT frontierSize = initFrontierSize; */
/*   intT lastFrontierSize = 0; */
/*   parallel_for(intT i=0;i<frontierSize;i++){ */
/*     intT v = Frontier[i]; */
/*     Visited[v] = NextVisited[v] = 1; */
/*   } */

/*   intT totalVisited = 0; */
/*   int round = 0; */

/*   bool fromSMV = 0; */

/*   {parallel_for (intT i=0; i < frontierSize; i++) */
/*       Counts[i] = G[Frontier[i]].degree;} */
/*   intT frontierEdgeCount = sequence::scan(Counts,Counts,frontierSize,utils::addF<intT>(),(intT)0); */

/*   while (!stoppingF(frontierSize)) { */
/*     round++; */
/*     totalVisited += frontierSize; */

/*     if(thresholdF(frontierSize+frontierEdgeCount)){ */
/*       if(fromSMV){ */
/* 	fromSMV = 0; */
/* 	pair<intT,intT> frontierInfo = getFrontierInfo(GA,Frontier,Visited,NextVisited,Counts); */
/* 	frontierSize = frontierInfo.first; */
/* 	frontierEdgeCount = frontierInfo.second; */
/*       } */

/*       cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-sparse method\n"; */

/*       // For each vertexB in the frontier try to "hook" unvisited neighbors. */
/*       {parallel_for(intT i = 0; i < frontierSize; i++) { */
/* 	  intT v = Frontier[i]; */
/* 	  intT o = Counts[i]; */
/* 	  for (intT j=0; j < G[v].degree; j++) { */
/* 	    intT ngh = G[v].Neighbors[j]; */
/* 	    if (NextVisited[ngh] == 0 && utils::CAS(&NextVisited[ngh],(intT)0,(intT)1)) */
/* 	      { */
/* 		FrontierNext[o+j] = ngh; */
/* 	      } */
/* 	    else FrontierNext[o+j] = -1; */
/* 	    if(Visited[ngh] == 0) Add.addAtomic(Vector[ngh],Vector[v]); */
/* 	  } */
/* 	}} */

/*       parallel_for(intT i=0;i<frontierSize;i++) { */
/* 	intT v = Frontier[i]; */
/* 	for(intT j=0;j<G[v].degree;j++){ */
/* 	  intT ngh = G[v].Neighbors[j]; */
/* 	  if(Visited[ngh] == 0) Visited[ngh] = 1; */
/* 	} */
/*       } */
      
	
/*       // Filter out the empty slots (marked with -1) */
/*       lastFrontierSize = frontierSize; */
/*       frontierSize = sequence::filter(FrontierNext,Frontier,frontierEdgeCount,nonNegF()); */

/*       parallel_for(intT i=0;i<frontierSize;i++) */
/*       	  Counts[i]=G[Frontier[i]].degree; */
/*       frontierEdgeCount = sequence::plusScan(Counts,Counts,frontierSize); */

/*     } */
/*     else { */
/*       //spsv */
/*       cout<<"round "<<round<<" frontierSize="<<frontierSize<<" frontierEdgeCount="<<frontierEdgeCount<<" sparse-dense method\n"; */

/*       parallel_for(intT i=0;i<numVertices;i++){ */
/* 	NextVisited[i] = Visited[i]; */
/* 	if(!NextVisited[i]){ */
/* 	  for(intT j=0;j<G[i].degree;j++){ */
/* 	    intT ngh = G[i].Neighbors[j]; */
/* 	    if(Visited[ngh]) { */
/* 	      Add(Vector[i],Vector[ngh]); */
/* 	      NextVisited[i] = 1; */
/* 	      //break; //only works for BFS */
/* 	    } */
/* 	  } */
/* 	} */
/*       } */

/*       frontierEdgeCount = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierDegreeDense(G,Visited,NextVisited)); */
      
/*       frontierSize = sequence::reduce<intT>((intT)0,numVertices,utils::addF<intT>(),getFrontierSize(G,Visited,NextVisited)); */
      
/*       if(thresholdF(frontierSize+frontierEdgeCount)) fromSMV=1; */
/*       else swap(NextVisited,Visited); */
/*     } */

/*   } */
/*   free(FrontierNext); free(Counts); */
/*   free(NextVisited); */

/* } */
