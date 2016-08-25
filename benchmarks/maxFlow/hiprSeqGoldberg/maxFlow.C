#include <cstring>
#include <cassert>
#include <limits>
#include <iostream>
#include "graph.h"
#include "gettime.h"
#include "utils.h"
#include "parallel.h"
#include "sequence.h"
#include "maxFlow.h"
#include "blockRadixSort.h"
using namespace std;

// implementation copied and adapted from
// http://www.avglab.com/andrew/soft.html
// http://www.avglab.com/andrew/soft/hipr.tar

/* Maximum flow - highest lavel push-relabel algorithm */
/* COPYRIGHT C 1995, 2000 by IG Systems, Inc., igsys@eclipse.net */

#define OTHER_WORK otherWork++ // undef for maximum performance
#define GLOB_REL_WORK glrelWork++ // undef for maximum performance

#define GLOB_UPDT_FREQ 0.5
#define ALPHA 6
#define BETA 12

#define WHITE 0
#define GREY 1
#define BLACK 2

typedef intT excessType;
typedef intT cType;

/* types */
struct node;
struct arc
{
   cType           resCap;          /* residual capasity */
   cType  revResCap;
   node   *head;           /* arc head */
   arc    *rev;            /* reverse arc */
};

struct node
{
   arc             *first;           /* first outgoing arc */
   arc             *current;         /* current outgoing arc */
   excessType      excess;           /* excess at the node
                                        change to double if needed */
   intT            d;                /* distance label */
   node   *bNext;           /* next node in bucket */
   node   *bPrev;           /* previous node in bucket */
};


struct bucket
{
  node             *firstActive;      /* first node with positive excess */
  node             *firstInactive;    /* first node with zero excess */
};

/* global variables */
intT   n;                    /* number of nodes */
intT   m;                    /* number of arcs */
intT   nm;                   /* n + ALPHA * m */
intT   nMin;                 /* smallest node id */
node   *nodes;               /* array of nodes */
arc    *arcs;                /* array of arcs */
bucket *buckets;             /* array of buckets */
cType  *cap;                 /* array of capacities */
node   *source;              /* source node pointer */
node   *sink;                /* sink node pointer */
intT   dMax;                 /* maximum label */
intT   aMax;                 /* maximum actie node label */
intT   aMin;                 /* minimum active node label */
intT flow;                   /* flow value */
intT pushCnt  = 0;           /* number of pushes */
intT relabelCnt   = 0;       /* number of relabels */
intT dischargeCnt = 0;
intT updateCnt    = 0;       /* number of updates */
intT gapCnt   = 0;           /* number of gaps */
intT gNodeCnt = 0;           /* number of nodes after gap */
intT gNodeExcess = 0;           /* excess of nodes after gap */
timer totalTime;
timer stageOneTime;
timer stageTwoTime;
timer updateTime;
float t, t2;                 /* for saving times */
node   *sentinelNode;        /* end of the node list marker */
arc *stopA;                  /* used in forAllArcs */
intT workSinceUpdate=0;      /* the number of arc scans since last update */
long long glrelWork, otherWork;
const float globUpdtFreq = GLOB_UPDT_FREQ;          /* global update frequency */

/* macros */

#define forAllNodes(i) for ( i = nodes; i != sentinelNode; i++ )
#define forAllArcs(i,a) for (a = i->first, stopA = (i+1)->first; a != stopA; a++)

#define nNode( i ) ( (i) - nodes + nMin )
#define nArc( a )  ( ( a == NULL )? -1 : (a) - arcs )

#define min( a, b ) ( ( (a) < (b) ) ? a : b )

intT i_dist;

#define aAdd(l,i)\
{\
  i->bNext = l->firstActive;\
  l->firstActive = i;\
  i_dist = i->d;\
  if (i_dist < aMin)\
    aMin = i_dist;\
  if (i_dist > aMax)\
    aMax = i_dist;\
  if (dMax < aMax)\
    dMax = aMax;\
}

/* i must be the first element */
#define aRemove(l,i)\
{\
  l->firstActive = i->bNext;\
}

node *i_next, *i_prev;
#define iAdd(l,i)\
{\
  i_next = l->firstInactive;\
  i->bNext = i_next;\
  i->bPrev = sentinelNode;\
  i_next->bPrev = i;\
  l->firstInactive = i;\
}

#define iDelete(l,i)\
{\
  i_next = i->bNext;\
  if (l->firstInactive == i) {\
    l->firstInactive = i_next;\
    i_next->bPrev = sentinelNode;\
  }\
  else {\
    i_prev = i->bPrev;\
    i_prev->bNext = i_next;\
    i_next->bPrev = i_prev;\
  }\
}

/* allocate datastructures, initialize related variables */

void allocDS( )
{
  nm = ALPHA * n + m;
  buckets = (bucket*) calloc ( n+2, sizeof (bucket) );
  sentinelNode = nodes + n;
  sentinelNode->first = arcs + 2*m;
} /* end of allocate */


void freeDS() {
  free(buckets);
}

void init1() {
  node  *i;        /* current node */
  bucket *l;
  arc *a;

  forAllNodes(i) {
    i->excess = 0;
    i->current = i->first;
    forAllArcs(i, a) {
      cap[a-arcs] = a->resCap;
      a->revResCap = a->rev->resCap;
    }
  }

  for (l = buckets; l <= buckets + n-1; l++) {
    l -> firstActive   = sentinelNode;
    l -> firstInactive  = sentinelNode;
  }


  aMax = 0;
  aMin = n;

  dMax = 1;
  pushCnt = 0;
  relabelCnt = 0;
  updateCnt = 0;
  gapCnt = 0;
  gNodeCnt = 0;
  glrelWork = otherWork = 0;
  totalTime.clear();
  stageOneTime.clear();
  stageTwoTime.clear();
  updateTime.clear();
}

void increaseArcFlow(arc* a, intT delta) {
  a -> resCap -= delta;
  (a -> rev) -> resCap += delta;
  a->revResCap = a->rev->resCap;
  a->rev->revResCap = a->resCap;
}

void init2( )
{
  node  *i;        /* current node */
  bucket *l;
  arc *a;
  intT delta;

  // initialize excesses

  source->excess = 0;
  forAllArcs(source,a) {
    if (a->head != source) {
      pushCnt ++;
      delta = a -> resCap;
      increaseArcFlow(a, a->resCap);
      a->head->excess += delta;
    }
  }

  l = buckets + 1;

  /*  setup labels and buckets */
  forAllNodes(i) {
    if (i == sink) {
      i->d = 0;
      iAdd(buckets,i);
      continue;
    }
    if (i == source) {
      i->d = n;
    }
    else
      i->d = 1;
    if (i->excess > 0) {
      /* put into active list */
      aAdd(l,i);
    }
    else { /* i -> excess == 0 */
      /* put into inactive list */
      if (i->d < n)
  iAdd(l,i);
    }
  }
} /* end of init */

void checkMax()
{
  bucket *l;

  for (l = buckets + dMax + 1; l < buckets + n; l++) {
    assert(l->firstActive == sentinelNode);
    assert(l->firstInactive == sentinelNode);
  }
}

/* global update via backward breadth first search from the sink */

void globalUpdate ()
{
  updateTime.start();
  node  *i, *j;       /* node pointers */
  arc   *a;           /* current arc pointers  */
  bucket *l, *jL;          /* bucket */
  intT curDist, jD;
  intT state;


  updateCnt ++;

  /* initialization */

  forAllNodes(i)
    i -> d = n;
  sink -> d = 0;

  for (l = buckets; l <= buckets + dMax; l++) {
    l -> firstActive   = sentinelNode;
    l -> firstInactive  = sentinelNode;
  }

  dMax = aMax = 0;
  aMin = n;

  /* breadth first search */

  // add sink to bucket zero

  iAdd(buckets, sink);
  for (curDist = 0; 1; curDist++) {

    state = 0;
    l = buckets + curDist;
    jD = curDist + 1;
    jL = l + 1;
    /*
    jL -> firstActive   = sentinelNode;
    jL -> firstInactive  = sentinelNode;
    */

    if ((l->firstActive == sentinelNode) &&
  (l->firstInactive == sentinelNode))
      break;

    while (1) {

      switch (state) {
      case 0:
  i = l->firstInactive;
  state = 1;
  break;
      case 1:
  i = i->bNext;
  break;
      case 2:
  i = l->firstActive;
  state = 3;
  break;
      case 3:
  i = i->bNext;
  break;
      default:
  assert(0);
  break;
      }

      if (i == sentinelNode) {
  if (state == 1) {
    state = 2;
    continue;
  }
  else {
    assert(state == 3);
    break;
  }
      }

      /* scanning arcs incident to node i */
      forAllArcs(i,a) {
        GLOB_REL_WORK;
        //assert(a->revResCap == a->rev->resCap);
        if (a->revResCap > 0 ) {
          j = a->head;
          if (j->d == n) {
            j->d = jD;
            j->current = j->first;
            if (jD > dMax) dMax = jD;

            if (j->excess > 0) {
              /* put into active list */
              aAdd(jL,j);
            }
            else {
              /* put into inactive list */
              iAdd(jL,j);
            }
          }
        }
      } /* node i is scanned */
    }
  }
  updateTime.stop();

} /* end of global update */

/* second stage -- preflow to flow */
void stageTwo ( )
/*
   do dsf in the reverse flow graph from nodes with excess
   cancel cycles if found
   return excess flow in topological order
*/

/*
   i->d is used for dfs labels
   i->bNext is used for topological order list
   buckets[i-nodes]->firstActive is used for DSF tree
*/
{
  node *i, *j, *tos, *bos, *restart, *r;
  arc *a;
  cType delta;

  /* deal with self-loops */
  forAllNodes(i) {
    forAllArcs(i,a)
      if ( a -> head == i ) {
  a -> resCap = cap[a - arcs];
      }
  }

  /* initialize */
  tos = bos = NULL;
  forAllNodes(i) {
    i -> d = WHITE;
    //    buckets[i-nodes].firstActive = NULL;
    buckets[i-nodes].firstActive = sentinelNode;
    i -> current = i -> first;
  }

  /* eliminate flow cycles, topologicaly order vertices */
  forAllNodes(i)
    if (( i -> d == WHITE ) && ( i -> excess > 0 ) &&
  ( i != source ) && ( i != sink )) {
      r = i;
      r -> d = GREY;
      do {
  for ( ; i->current != (i+1)->first; i->current++) {
    a = i -> current;
    if (( cap[a - arcs] == 0 ) && ( a -> resCap > 0 )) {
      j = a -> head;
      if ( j -> d == WHITE ) {
        /* start scanning j */
        j -> d = GREY;
        buckets[j-nodes].firstActive = i;
        i = j;
        break;
      }
      else
        if ( j -> d == GREY ) {
    /* find minimum flow on the cycle */
    delta = a -> resCap;
    while ( 1 ) {
      delta = min ( delta, j -> current -> resCap );
      if ( j == i )
        break;
      else
        j = j -> current -> head;
    }

    /* remove delta flow units */
    j = i;
    while ( 1 ) {
      a = j -> current;
      a -> resCap -= delta;
      a -> rev -> resCap += delta;
      j = a -> head;
      if ( j == i )
        break;
    }

    /* backup DFS to the first saturated arc */
    restart = i;
    for ( j = i -> current -> head; j != i; j = a -> head ) {
      a = j -> current;
      if (( j -> d == WHITE ) || ( a -> resCap == 0 )) {
        j -> current -> head -> d = WHITE;
        if ( j -> d != WHITE )
          restart = j;
      }
    }

    if ( restart != i ) {
      i = restart;
      i->current++;
      break;
    }
        }
    }
  }

  if (i->current == (i+1)->first) {
    /* scan of i complete */
    i -> d = BLACK;
    if ( i != source ) {
      if ( bos == NULL ) {
        bos = i;
        tos = i;
      }
      else {
        i -> bNext = tos;
        tos = i;
      }
    }

    if ( i != r ) {
      i = buckets[i-nodes].firstActive;
      i->current++;
    }
    else
      break;
  }
      } while ( 1 );
    }


  /* return excesses */
  /* note that sink is not on the stack */
  if ( bos != NULL ) {
    for ( i = tos; i != bos; i = i -> bNext ) {
      a = i -> first;
      while ( i -> excess > 0 ) {
  if (( cap[a - arcs] == 0 ) && ( a -> resCap > 0 )) {
    if (a->resCap < i->excess)
      delta = a->resCap;
    else
      delta = i->excess;
    a -> resCap -= delta;
    a -> rev -> resCap += delta;
    i -> excess -= delta;
    a -> head -> excess += delta;
  }
  a++;
      }
    }
    /* now do the bottom */
    i = bos;
    a = i -> first;
    while ( i -> excess > 0 ) {
      if (( cap[a - arcs] == 0 ) && ( a -> resCap > 0 )) {
  if (a->resCap < i->excess)
    delta = a->resCap;
  else
    delta = i->excess;
  a -> resCap -= delta;
  a -> rev -> resCap += delta;
  i -> excess -= delta;
  a -> head -> excess += delta;
      }
      a++;
    }
  }
}


/* gap relabeling */

int gap (bucket * emptyB)
{
  bucket *l;
  node  *i;
  intT  r;           /* index of the bucket before l  */
  int   cc;          /* cc = 1 if no nodes with positive excess before
          the gap */

  gapCnt ++;
  r = ( emptyB - buckets ) - 1;

  /* set labels of nodes beyond the gap to "infinity" */
  for ( l = emptyB + 1; l <= buckets + dMax; l ++ ) {
    // this does nothing for high level selection
    // but is needed for wave initialization
#ifdef WAVE_INIT
    for (i = l -> firstActive; i != sentinelNode; i = i -> bNext) {
      i -> d = n;
      gNodeCnt++;
      gNodeExcess += i->excess;
    }
    l -> firstActive = sentinelNode;
#endif
    for ( i = l -> firstInactive; i != sentinelNode; i = i -> bNext ) {
      i -> d = n;
      gNodeCnt ++;
      gNodeExcess += i->excess;
    }

    l -> firstInactive = sentinelNode;
  }

  cc = ( aMin > r ) ? 1 : 0;

  dMax = r;
  aMax = r;

  return ( cc );
}

/*--- relabelling node i */

intT relabel (node *i)
{
  node  *j;
  intT  minD;     /* minimum d of a node reachable from i */
  arc   *minA;    /* an arc which leads to the node with minimal d */
  arc   *a;

  assert(i->excess > 0);

  relabelCnt++;
  workSinceUpdate += BETA;

  i->d = minD = n;
  minA = NULL;

  /* find the minimum */
  forAllArcs(i,a) {
    workSinceUpdate++;
    OTHER_WORK;
    if (a -> resCap > 0) {
      j = a -> head;
      if (j->d < minD) {
  minD = j->d;
  minA = a;
      }
    }
  }

  minD++;

  if (minD < n) {

    i->d = minD;
    i->current = minA;

    if (dMax < minD) dMax = minD;

  } /* end of minD < n */

  return ( minD );

} /* end of relabel */


/* discharge: push flow out of i until i becomes inactive */

void discharge (node *i)
{
  node  *j;                 /* sucsessor of i */
  intT  jD;                 /* d of the next bucket */
  bucket *lj;               /* j's bucket */
  bucket *l;                /* i's bucket */
  arc   *a;                 /* current arc (i,j) */
  cType  delta;
  arc *stopA;

  dischargeCnt++;
  assert(i->excess > 0);
  assert(i != sink);
  do {

    jD = i->d - 1;
    l = buckets + i->d;

    /* scanning arcs outgoing from  i  */
    for (a = i->current, stopA = (i+1)->first; a != stopA; a++) {
      OTHER_WORK;
      if (a -> resCap > 0) {
  j = a -> head;

  if (j->d == jD) {
    pushCnt ++;
    if (a->resCap < i->excess)
      delta = a->resCap;
    else
      delta = i->excess;
    increaseArcFlow(a, delta);

    if (j != sink) {

      lj = buckets + jD;

      if (j->excess == 0) {
        /* remove j from inactive list */
        iDelete(lj,j);
        /* add j to active list */
        aAdd(lj,j);
      }
    }

    j -> excess += delta;
    i -> excess -= delta;

    if (i->excess == 0) break;

  } /* j beintTs to the next bucket */
      } /* a  is not saturated */
    } /* end of scanning arcs from  i */

    if (a == stopA) {
      /* i must be relabeled */
      relabel (i);

      if (i->d == n) break;
      if ((l -> firstActive == sentinelNode) &&
    (l -> firstInactive == sentinelNode)
    )
  gap (l);

      if (i->d == n) break;
    }
    else {
      /* i no intTer active */
      i->current = a;
      /* put i on inactive list */
      iAdd(l,i);
      break;
    }
  } while (1);
}


// go from higher to lower buckets, push flow
void wave() {
  node   *i;
  bucket  *l;

  for (l = buckets + aMax; l > buckets; l--) {
    for (i = l->firstActive; i != sentinelNode; i = l->firstActive) {
      aRemove(l,i);

      assert(i->excess > 0);
      discharge (i);

    }
  }
}


/* first stage  -- maximum preflow*/

template <typename T> bool nonNegF(T x) { return x >= 0; }

typedef long long ll;
void stageOne ( )
{
  node   *i;
  bucket  *l;             /* current bucket */


#if defined(INIT_UPDATE) || defined(OLD_INIT) || defined(WAVE_INIT)
  globalUpdate ();
#endif

  workSinceUpdate = 0;

#ifdef WAVE_INIT
  wave();
#endif

  /* main loop */
  while ( aMax >= aMin ) {
    l = buckets + aMax;
    i = l->firstActive;

    if (i == sentinelNode)
      aMax--;
    else {
      aRemove(l,i);

      assert(i->excess > 0);
      discharge (i);

      if (aMax < aMin)
  break;

      /* is it time for global update? */
      if (workSinceUpdate * globUpdtFreq > nm) {
  globalUpdate ();
  workSinceUpdate = 0;
      }

    }

  } /* end of the main loop */

  flow = sink -> excess;
}

intT maxFlow(FlowGraph<intT>& g) {
  parallel_for (intT i = 1; i < n; ++i) {
    assert(g.g.V[i].Neighbors == g.g.V[i - 1].Neighbors + g.g.V[i - 1].degree);
    assert(g.g.V[i].nghWeights == g.g.V[i - 1].nghWeights + g.g.V[i - 1].degree);
  }
  intT *adj = g.g.V[0].Neighbors, *weights = g.g.V[0].nghWeights;

  timer timeInit; timeInit.start();
  n = g.g.n, m = 2 * g.g.m;
  nodes = new node[n + 1];
  arcs = new arc[m];
  source = nodes + g.source;
  sink = nodes + g.sink;

  timer t; t.start();
  nodes[n].first = arcs + m;
  cap = new intT[m];
  intT *edges = new intT[m];
  parallel_for (intT i = 0; i < n; ++i) {
    nodes[i].first = 0;
    FlowVertex& v = g.g.V[i];
    intT offset = v.Neighbors - adj;
    parallel_for (intT j = 0; j < v.degree; ++j) {
      intT pairId = offset + j;
      cap[pairId] = v.nghWeights[j];
      v.nghWeights[j] = i;
    }
  }
  parallel_for (intT i = 0; i < m; ++i) edges[i] = i;
  auto toEdge = [&] (intT idx) {
    intT i = weights[idx / 2];
    intT j = adj[idx / 2];
    if (idx & 1) swap(i, j);
    return make_pair(i, j);
  };
  t.stop(); cout << "t1 = " << t.total() << endl; t.clear(); t.start();
  static_assert(sizeof(__int128) >= 2 * sizeof(intT), "Need to be able to square");
  intSort::iSort(edges, m, (__int128)(n - 1) * (n + 1) + 1,
        [&] (intT x) {
          pair<intT,intT> fromTo = toEdge(x);
          return (__int128)fromTo.first * n + fromTo.second;
        });

  intT *firstTmp = new intT[m], *first = new intT[m];
  firstTmp[0] = 0;
  parallel_for (intT i = 1; i < m; ++i)
    firstTmp[i] = (toEdge(edges[i]).first != toEdge(edges[i - 1]).first) ? i : -1;
  intT sz = sequence::filter(firstTmp, first, m, nonNegF<intT>);
  parallel_for (intT i = 0; i < sz; ++i)
    nodes[toEdge(edges[first[i]]).first].first = arcs + first[i];
  t.stop(); cout << "t2 = " << t.total() << endl; t.clear(); t.start();
  delete[] firstTmp;
  delete[] first;

  for (intT i = n - 1; i >= 0; --i)
    if (!nodes[i].first) nodes[i].first = nodes[i + 1].first;
  t.stop(); cout << "t3 = " << t.total() << endl; t.clear(); t.start();

  intT* pos = new intT[m];
  parallel_for (intT i = 0; i < m; ++i)
    pos[edges[i]] = i;
  t.stop(); cout << "t4 = " << t.total() << endl; t.clear(); t.start();
  parallel_for (intT i = 0; i < m; ++i) {
    intT idx = edges[i];
    pair<intT, intT> fromTo = toEdge(idx);
    intT pairId = idx / 2;
    arcs[i].rev = arcs + pos[idx ^ 1];
    arcs[i].head = nodes + fromTo.second;
    arcs[i].resCap = (idx & 1) ? 0 : cap[idx / 2];
  }
  delete[] pos;
  t.stop(); cout << "t5 = " << t.total() << endl; t.clear(); t.start();

  parallel_for (intT i = 0; i < m; ++i) {
    intT pairId = edges[i] / 2;
    if (edges[i] & 1) weights[pairId] = i;
  }
  delete[] edges;
  m /= 2;

  //n = g.g.n, m = 2 * g.g.m;
  //timer timeInit; timeInit.start();
  //nodes = new node[n + 1];
  //arcs = new arc[m];
  //source = nodes + g.source;
  //sink = nodes + g.sink;

  //intT *offset = new intT[n + 1];
  //memset(offset, 0, (n + 1) * sizeof *offset);
  //auto *edges = new tuple<intT, intT, intT>[m];
  //parallel_for (intT i = 0; i < n; ++i)
    //offset[i] = g.g.V[i].degree;
  //sequence::plusScan(offset, offset, n);
  //nodes[n].first = arcs + m;

  //auto *arcPool = new arc[m];
  //intT **finalLocation = new intT*[m / 2];
  //parallel_for (intT i = 0; i < n; ++i) {
    //nodes[i].first = 0;
    //FlowVertex& v = g.g.V[i];
    //parallel_for (intT j = 0; j < v.degree; ++j) {
      //intT w = v.Neighbors[j];
      //intT c = v.nghWeights[j];
      //intT pairId = offset[i] + j;
      //intT o = 2 * pairId;
      //arcPool[o].resCap = c;
      //arcPool[o].head = nodes + w;
      //arcPool[o + 1].resCap = 0;
      //arcPool[o + 1].head = nodes + i;
      //edges[o] = (tuple<intT,intT,intT>) { i, w, o };
      //edges[o + 1] = (tuple<intT,intT,intT>) { w, i, o + 1 };
      //finalLocation[pairId] = v.nghWeights + j;
    //}
  //}
  //assert(sizeof(ll) >= 2 * sizeof(intT));
  //intSort::iSort(edges, m, (ll)(n - 1) * (n + 1) + 1,
        //[&] (decltype(*edges)& x) -> ll { return (ll)get<0>(x) * n + get<1>(x); });

  //parallel_for (intT i = 0; i < m; ++i) {
    //intT idx = get<2>(edges[i]);
    //arcPool[idx ^ 1].rev = arcs + i;
    //if (idx & 1) *finalLocation[idx / 2] = i;
  //}
  //parallel_for (intT i = 0; i < m; ++i) {
    //intT idx = get<2>(edges[i]);
    //arcs[i] = arcPool[idx];
  //}
  //delete[] finalLocation;

  //intT *firstTmp = new intT[m], *first = new intT[m];
  //firstTmp[0] = 0;
  //parallel_for (intT i = 1; i < m; ++i)
    //firstTmp[i] = (get<0>(edges[i]) != get<0>(edges[i - 1])) ? i : -1;
  //intT sz = sequence::filter(firstTmp, first, m, nonNegF<intT>);

  //parallel_for (intT i = 0; i < sz; ++i)
    //nodes[get<0>(edges[first[i]])].first = arcs + first[i];

  //for (intT i = n - 1; i >= 0; --i)
    //if (!nodes[i].first) nodes[i].first = nodes[i + 1].first;

  //delete[] edges, firstTmp, first, arcPool, offset;

  //n = g.g.n, m = g.g.m;
  //nodes = new node[n + 1];
  //arcs = new arc[2 * m];
  //cap = new cType[2 * m];
  //source = nodes + g.source;
  //sink = nodes + g.sink;
  //intT *offset = new intT[n + 1];
  //memset(offset, 0, (n + 1) * sizeof *offset);
  //for (intT i = 0; i < n; ++i) {
    //FlowVertex& v = g.g.V[i];
    //offset[i] += v.degree;
    //for (intT j = 0; j < v.degree; ++j)
      //offset[v.Neighbors[j]]++;
  //}
  //intT o = 0;
  //for (intT i = 0; i <= n; ++i) {
    //o += offset[i];
    //offset[i] = o - offset[i];
    //nodes[i].first = arcs + offset[i];
  //}
  //for (intT i = 0; i < n; ++i) {
    //FlowVertex& v = g.g.V[i];
    //for (intT j = 0; j < v.degree; ++j) {
      //intT w = v.Neighbors[j];
      //intT c = v.nghWeights[j];
      //arcs[offset[i]].head = nodes + w;
      //arcs[offset[i]].rev = arcs + offset[w];
      //arcs[offset[i]].resCap = c;
      //arcs[offset[w]].head = nodes + i;
      //arcs[offset[w]].rev = arcs + offset[i];
      //arcs[offset[w]].resCap = 0;
      //v.nghWeights[j] = offset[w];
      //offset[i]++;
      //offset[w]++;
    //}
  //}

  allocDS();

  timeInit.stop();
  init1();
  cout << "init time: " << timeInit.total() << endl;
  beforeHook();
  totalTime.start();
  init2();

  stageOneTime.start();
  stageOne();
  stageOneTime.stop();
  afterHook();

  stageTwoTime.start();
  stageTwo();
  stageTwoTime.stop();

  totalTime.stop();

  for (intT i = 0; i < n; ++i) {
    FlowVertex& v = g.g.V[i];
    for (intT j = 0; j < v.degree; ++j)
      v.nghWeights[j] = arcs[v.nghWeights[j]].resCap;
  }
  delete[] nodes;
  delete[] arcs;
  delete[] cap;
  freeDS();
  cout << setprecision(4) << fixed;
  cout << "========== Stats =================" << endl;
  cout << "  #nodes      " << setw(20) << n << endl;
  cout << "  #edges      " << setw(20) << m << endl;
  cout << "  #pushes     " << setw(20) << pushCnt << endl;
  cout << "  #relabels   " << setw(20) << relabelCnt << endl;
  cout << "  rl / dc     " << setw(20) << setprecision(2)  << fixed
                        << (double)relabelCnt/dischargeCnt << endl;
  cout << "  #p + #r     " << setw(20) << pushCnt + relabelCnt << endl;
  cout << "  #glrel      " << setw(20) << updateCnt << endl;
  cout << "  #gaps       " << setw(20) << gapCnt << endl;
  cout << "  #gap nodes  " << setw(20) << gNodeCnt << endl;
  cout << "  gap excess  " << setw(20) << gNodeExcess << endl;
  cout << "  glrel work  " << setw(20) << glrelWork << endl;
  cout << "  total work  " << setw(20) << glrelWork + otherWork << endl;
  cout << endl;
  cout << "  total time  " << setw(20) << totalTime.total() << endl;
  cout << "  glrel time  " << setw(20) << updateTime.total() << endl;
  cout << "  stage1 time " << setw(20) << stageOneTime.total() << endl;
  cout << "  stage2 time " << setw(20) << stageTwoTime.total() << endl;
  cout << endl;
  cout << "  flow        " << setw(20) << flow << endl;
  cout << "==================================" << endl;
  return flow;
}
