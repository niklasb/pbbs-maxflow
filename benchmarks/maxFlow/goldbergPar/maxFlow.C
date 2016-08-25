// config
//#define SEQUENTIAL
//#define NDEBUG
#define INITIAL_RELABEL 1
#define DELAY_EXCESS_INCREASE 1
#define ADMISSIBLE_EDGE_DS 0
#define SORT_DESC_BY_LABEL 0
#define GAP_HEURISTIC 0
#define BFS_METHOD 2 // 1 = serial FIFO, 2 = adjarray pack, 3 = pbfs
#define LABEL_COUNTER_METHOD 2 // 1 = read-modify-write, 2 = reduce
const bool minCutOnly = 1; // must be 1 for gap heuristic
const bool stats = 0;
const int statsInterval = 1000;
const float globUpdtFreq = 3.0;

#define ALPHA 6
#define BETA 12

#ifdef SEQUENTIAL
//#  undef CILK
//#  undef CILKP
//#  undef OPENMP
#  include "atomic_nop.h"
#else
#  include "atomic_x86.h"
//#  include <atomic>
#endif

#include <iostream>
#include <queue>
#if !OPENMP
#include <cilk/reducer_opadd.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_ostream.h>
using namespace cilk;
#endif
#include "graph.h"
#include "gettime.h"
#include "utils.h"
#include "sequence.h"
#include "parallel.h"
#include "maxFlow.h"
#include "bag.h"
#include "blockRadixSort.h"
using namespace std;
typedef long long ll;

#if OPENMP
#  include <parallel/algorithm>
#  define COMP_SORT __gnu_parallel::sort
#else
#  define COMP_SORT sort
#endif

const intT inf = numeric_limits<intT>::max();

#ifdef NDEBUG
#  define ass2(cond, msg)
#  define ass(cond)
#else
#  define ass2(cond, msg) if (!(cond)) \
    cerr << __FILE__ <<":" <<__LINE__<<": assertion '"  #cond << "' failed. " \
           << msg << endl, abort()
#  define ass(cond) ass2(cond, "")
#endif

void sortInts(intT* A, intT n, intT maxV) {
  intSort::iSort(A, n, maxV + 1, utils::identityF<intT>());
}
template<typename T>
void sortByFstInts(pair<intT,T>* A, intT n, intT maxV) {
  intSort::iSort(A, n, maxV + 1, utils::firstF<intT,T>());
}

template<typename T> bool nonNegF(T x) { return x >= 0; }

intT groupInts(intT* A, intT* first, intT* firstOut, intT n, intT maxV) {
  sortInts(A, n, maxV);
  first[0] = 0;
  parallel_for (intT i = 1; i < n; ++i)
    first[i] = (A[i] != A[i - 1]) ? i : -1;
  intT sz = sequence::filter(first, firstOut, n, nonNegF<intT>);
  firstOut[sz] = n;
  return sz;
}

// implementation of "Efficient Graph Algorithms for Sequential and Parallel
// Computers", Chapter 1.6, Andrew V. Goldberg, 1987
struct Arc {
  intT resCap;
  intT to;
  intT rev;
  intT revResCap;
} *arcs;

struct Node {
  intT first, cur;
#if DELAY_EXCESS_INCREASE
  intT e;
  atomic<intT> eadd;
#else
  atomic<intT> e;
#endif
  intT h;
  intT *buf;
  intT lastRelabelTime; // value < 0 indicates the node is removed
} *nodes;

intT last(Node& v) { return (&v + 1)->first; }
intT outDegree(Node& v) { return last(v) - v.first; }

#if DELAY_EXCESS_INCREASE
  intT totalExcess(Node& v) { return v.e + v.eadd; }
#else
  intT totalExcess(Node& v) { return v.e; }
#endif
#define for_arcs(for, n, a, body) \
  {for (intT _i = (n).first; _i < last(n); ++_i) { Arc& a = arcs[_i]; body }}

intT n, m, source, sink;
intT *nodeBuffer;
intT *sizes, *offset;
#if LABEL_COUNTER_METHOD == 1
  atomic<intT> *labelCounter;
#else
  intT *labelCounter;
#endif
intT *newLabel, *oldLabel, *first, *firstTmp;
intT *ignoreNode;
pair<intT, intT> *nodesByLabel;
intT hiLabel;
intT gap;
ll relabels, pushes;
ll globalRelabels;
ll workSinceUpdate;
timer globalRelabelTime, totalTime, timePhase1, timePhase2, timePhase3, gapHeuristicTime;
double totActiveNodes;
intT gaps;
intT pass;
intT lastGapTime;
ll globRelWork, otherWork;
intT removedByGap;

struct Frontier {
  intT *curNodeSet, *newNodeSet;
  intT curNodeSetSz, newNodeSetSz;
  atomic_flag *inNewNodeSet;
  void init() {
    inNewNodeSet = new atomic_flag[n];
    curNodeSet = new intT[n];
    newNodeSet = new intT[n];
    parallel_for (intT i = 0; i < n; ++i)
      inNewNodeSet[i].clear();
    newNodeSetSz = 0;
  }
  bool empty() {
    return !curNodeSetSz;
  }
  void del() {
    delete[] curNodeSet;
    delete[] newNodeSet;
    delete[] inNewNodeSet;
  }
  void addNode_(intT wi, intT*& it) { *it++ = wi; }
  bool addNode(intT wi, intT*& it) {
    if (!inNewNodeSet[wi].test_and_set()) {
      addNode_(wi, it);
      return 1;
    }
    return 0;
  }
  void reportEnd(intT i, intT*& it) {
    intT vi = curNodeSet[i];
    sizes[i] = it - nodes[vi].buf;
  }
  void pack() {
    //cout << "sizes = ";
    //for (intT i = 0; i < curNodeSetSz; ++i) cout << sizes[i] << " "; cout << endl;
    newNodeSetSz = sequence::plusScan(sizes, offset, curNodeSetSz);
    //cout << "newNodeSetSz = " << newNodeSetSz << endl;
    //cout << "sizes = ";
    //for (intT i = 0; i < curNodeSetSz; ++i) cout << sizes[i] << " "; cout << endl;
    parallel_for (intT i = 0; i < curNodeSetSz; ++i) {
      Node& v = nodes[curNodeSet[i]];
      copy(v.buf, v.buf + sizes[i], newNodeSet + offset[i]);
    }
  }
  void clearNodeSetFlags() {
    parallel_for (intT i = 0; i < curNodeSetSz; ++i)
      inNewNodeSet[curNodeSet[i]].clear();
  }
  void switchSets(bool clear= 1) {
    swap(curNodeSet, newNodeSet);
    curNodeSetSz = newNodeSetSz;
    newNodeSetSz = 0;
    if (clear) clearNodeSetFlags();
  }
};

Frontier prFrontier, bfsFrontier;

void init() {
  // n, m, source, sink and the graph structure stored in nodes, arcs must be
  // initialized already
  sizes = new intT[n];
  offset = new intT[n];
  nodeBuffer = new intT[m + n];
#if LABEL_COUNTER_METHOD == 1
  labelCounter = new atomic<intT>[2 * n + 1];
#else
  labelCounter = new intT[2 * n + 1];
#endif
  prFrontier.init();
  bfsFrontier.init();
  parallel_for (intT i = 0; i <= 2 * n; ++i) labelCounter[i] = 0;
  labelCounter[0] = n - 1;
  for (intT i = 0; i < n; ++i) {
    nodes[i].h = 0;
    nodes[i].e = 0;
#if DELAY_EXCESS_INCREASE
    nodes[i].eadd = 0;
#endif
    nodes[i].cur = nodes[i].first;
    nodes[i].lastRelabelTime = 0;
    nodes[i].buf =
        (i > 0) ? (nodes[i - 1].buf + outDegree(nodes[i - 1]) + 1)
                : nodeBuffer;
  }
  parallel_for (intT i = 0; i < m; ++i) {
    arcs[i].revResCap = arcs[arcs[i].rev].resCap;
  }
  oldLabel = new intT[n];
  newLabel = new intT[n];
  first = new intT[n + 1];
  firstTmp = new intT[n + 1];
  ignoreNode = new intT[n];
  nodesByLabel = new pair<intT, intT>[n];
}

void deinit() {
  delete[] sizes;
  delete[] nodeBuffer;
  delete[] labelCounter;
  delete[] oldLabel;
  delete[] newLabel;
  delete[] first;
  delete[] firstTmp;
  delete[] ignoreNode;
  delete[] nodesByLabel;
  prFrontier.del();
  bfsFrontier.del();
}

void debugState() {
  cout << "gap = " << gap << endl;
  cout << "e = ";
  for (intT i = 0; i < n; ++i) cout << nodes[i].e << " "; cout << endl;
#if DELAY_EXCESS_INCREASE
  cout << "eadd = ";
  for (intT i = 0; i < n; ++i) cout << nodes[i].eadd << " "; cout << endl;
#endif
  cout << "h = ";
  for (intT i = 0; i < n; ++i) cout << nodes[i].h << " "; cout << endl;
  //cout << "newh = ";
  //for (intT i = 0; i < n; ++i) cout << nodes[i].newh << " "; cout << endl;
  cout << "active nodes = ";
  for (intT i = 0; i < prFrontier.curNodeSetSz; ++i)
    cout << prFrontier.curNodeSet[i] << " ";
  cout << endl;
}

inline bool nodeRelevant(Node& v) {
  if (!minCutOnly) return 1;
  return v.h < hiLabel;
#if !GAP_HEURISTIC
  return v.h < hiLabel;
#endif
  //if (v.lastRelabelTime < 0) return 0;
  //TODO make more efficient (by memoizing :)
  if (v.lastRelabelTime < 0) return 0;
  if (v.h >= hiLabel ||
         v.h >= gap && v.lastRelabelTime <= lastGapTime) {
    //cout << "removing " << (&v - nodes) << endl;
    //ass2(0, v.h << " " << gap <<" " << v.lastRelabelTime << " " << lastGapTime);
    v.lastRelabelTime = -1;
    if (stats) removedByGap++;
    return 0;
  }
  return 1;
}

void checkLabelCounter() {
  static atomic<intT> *lblCnt;
  if (!lblCnt) lblCnt = new atomic<intT>[2 * n + 1];
  parallel_for (intT i = 0; i < hiLabel; ++i) lblCnt[i] = labelCounter[i];
  parallel_for (intT i = 0; i < n; ++i) lblCnt[nodes[i].h]--;
  parallel_for (intT i = 0; i < hiLabel; ++i) ass(!lblCnt[i]);
}

void checkDistanceLabels() {
  parallel_for (intT i = 0; i < n; ++i) {
    for_arcs(parallel_for, nodes[i], a, {
      if (a.resCap > 0 && nodeRelevant(nodes[i])) {
        ass2(nodes[i].h <= nodes[a.to].h + 1,
               i << " " << a.to << " " << nodes[i].h << " " << nodes[a.to].h);
      }
    })
  }
}

void checkSanity() {
  ass(nodes[source].h == n);
  ass(nodes[sink].h == 0);
  static atomic_flag *flag;
  if (!flag) {
    flag = new atomic_flag[n];
    parallel_for (intT i = 0; i < n; ++i) flag[i].clear();
  }
  parallel_for (intT i = 0; i < prFrontier.curNodeSetSz; ++i) {
    ass(!flag[prFrontier.curNodeSet[i]].test_and_set());
  }
  static atomic<intT> *exc;
  if (!exc) exc = new atomic<intT>[n];
  parallel_for (intT i = 0; i < n; ++i) exc[i] = 0;
  ass(!flag[source].test_and_set());
  ass(!flag[sink].test_and_set());
  checkDistanceLabels();
#if GAP_HEURISTIC
  checkLabelCounter();
#endif
  parallel_for (intT i = 0; i < n; ++i) {
    if (i != source && i != sink) {
#if DELAY_EXCESS_INCREASE // if we use this method, we can maintain the precise set of active nodes
      if (nodeRelevant(nodes[i]))
        ass(!!nodes[i].e == flag[i].test_and_set());
#else
      if (nodeRelevant(nodes[i]) && nodes[i].e) ass(flag[i].test_and_set());
#endif
    }
    flag[i].clear();
    Node& v = nodes[i];
    for_arcs(parallel_for, v, a, {
      Node& w = nodes[a.to];
      ass(a.resCap == arcs[a.rev].revResCap);
      ass(a.revResCap == arcs[a.rev].resCap);
      ass(a.resCap >= 0);
      ass(arcs[a.rev].resCap >= 0);
      //if (a.isorig) {
        //intT f = a.revResCap;
        //ass(a.resCap + f == cap(a));
        //exc[i] -= f;
        //exc[a.to] += f;
      //}
    })
  }
  //parallel_for (intT i = 0; i < n; ++i) {
    //if (i != source && i != sink) {
      //if (nodes[i].e != exc[i]) {
        //cout << i << " " << nodes[i].e << " " << exc[i] << endl;
      //ass(nodes[i].e == exc[i]);
      //}
    //}
  //}
}

template<typename F>
void revBfsSerial(intT start, intT exclude, F heightF, bool maintainGap) {
  //cout << "start=" << start << endl;
  static queue<intT> q;
  ass(q.empty());
  q.push(start);
  while (!q.empty()) {
    intT wi = q.front();
    if (maintainGap) {
      intT d = heightF(nodes[wi]);
      gap = d + 1;
      labelCounter[d]++;
    }
    q.pop();
    Node& w = nodes[wi];
    ass2(w.lastRelabelTime >= 0, "node: " << wi);
    for_arcs(for, w, ar, {
      if (stats) globRelWork++;
      if (!ar.revResCap) continue;
      intT vi = ar.to;
      if (vi == exclude) continue;
      Node& v = nodes[vi];
      if (heightF(v) > heightF(w) + 1) {
        //cout << "relax " << vi << " -> " << wi << endl;
        heightF(v) = heightF(w) + 1;
        q.push(vi);
      }
    })
  }
}

#if BFS_METHOD == 1

#if GAP_HEURISTIC
  const bool gapHeuristic = 1;
#else
  const bool gapHeuristic = 0;
#endif

void revBfs(intT start, intT exclude) {
  revBfsSerial(start, exclude, [&] (Node& v) -> intT& { return v.h; }, gapHeuristic);
}

#elif BFS_METHOD == 2

void revBfs(intT start, intT exclude) {
  ass(bfsFrontier.newNodeSetSz == 0);
  bfsFrontier.newNodeSet[bfsFrontier.newNodeSetSz++] = start;
  bfsFrontier.switchSets();
  intT d = nodes[start].h;
#if !OPENMP
  reducer_opadd<ll> addWork;
#endif
  while (!bfsFrontier.empty()) {
    intT *curNodeSet = bfsFrontier.curNodeSet;
    intT sz = bfsFrontier.curNodeSetSz;
    //cout << "level " << d << " nodes " << sz << endl;
#if GAP_HEURISTIC
    gap = d + 1;
    labelCounter[d] = sz;
#endif
    if (stats) {
#if !OPENMP
      parallel_for (intT i = 0; i < sz; ++i)
        addWork += outDegree(nodes[curNodeSet[i]]);
#endif
    }
    parallel_for (intT i = 0; i < sz; ++i) {
      intT wi = curNodeSet[i];
      Node& w = nodes[wi];
      ass(w.lastRelabelTime >= 0);
      intT *out = w.buf;
      for_arcs(for, w, ar, {
        if (!ar.revResCap) continue;
        intT vi = ar.to;
        if (vi == exclude) continue;
        if (CAS(&nodes[vi].h, hiLabel, w.h + 1))
          bfsFrontier.addNode_(vi, out);
      })
      bfsFrontier.reportEnd(i, out);
    }
    bfsFrontier.pack();
    bfsFrontier.switchSets();
    d++;
  }
#if !OPENMP
  if (stats) globRelWork += addWork.get_value();
#endif
}

#elif BFS_METHOD == 3

#define NODES_PER_ITER 256

intT excludeNode;
reducer_opadd<ll> addWork;

static inline void
pbfs_proc_Node(const intT n[], intT fillSize,
               Bag_reducer<intT> *next, reducer_opadd<ll>& work)
{
  // Process the current element
  Bag<intT>* bnext = &((*next).get_reference());
  for (intT j = 0; j < fillSize; ++j) {
    // Scan the edges of the current node and add untouched
    // neighbors to the opposite bag
    Node& w = nodes[n[j]];
    ass2(w.lastRelabelTime >= 0, n[j] << " " << nodes[n[j]].h);
    if (stats) work += outDegree(w);
    for_arcs(for, w, ar, {
      if (!ar.revResCap) continue;
      intT vi = ar.to;
      if (vi == excludeNode) continue;
      Node& v = nodes[vi];
      if (CAS(&v.h, hiLabel, w.h + 1)) {
      //if (v.h == hiLabel) { // unvisited vertex
        //v.h = w.h + 1;
        (*bnext).insert(vi);
      }
    })
  }
}

inline void
pbfs_walk_Pennant(Pennant<intT> *p, Bag_reducer<intT>* next,
                  reducer_opadd<ll>& work)
{
  if (p->getLeft())
    cilk_spawn pbfs_walk_Pennant(p->getLeft(), next, work);

  if (p->getRight())
    cilk_spawn pbfs_walk_Pennant(p->getRight(), next, work);

  // Process the current element
  const intT *n = p->getElements();

  parallel_for_1 (intT i = 0; i < BLK_SIZE; i+=NODES_PER_ITER) {
    // This is fine as long as NODES_PER_ITER divides BLK_SIZE
    pbfs_proc_Node(n+i, NODES_PER_ITER, next, work);
  }
  delete p;
}


inline void
pbfs_walk_Bag(Bag<intT> *b, Bag_reducer<intT>* next, reducer_opadd<ll>& work)
{
  if (b->getFill()) {
    // Split the bag and recurse
    Pennant<intT> *p = 0;

    b->split(&p); // Destructive split, decrements b->getFill()
    cilk_spawn pbfs_walk_Bag(b, next, work);
    pbfs_walk_Pennant(p, next, work);
    cilk_sync;
  } else {
    intT fillSize = b->getFillingSize();
    const intT *n = b->getFilling();

    intT extraFill = fillSize % NODES_PER_ITER;
    cilk_spawn pbfs_proc_Node(n+fillSize-extraFill, extraFill, next, work);
    parallel_for_1 (intT i = 0; i < fillSize - extraFill; i += NODES_PER_ITER) {
      pbfs_proc_Node(n+i, NODES_PER_ITER, next, work);
    }
    cilk_sync;
  }
}

void revBfs(intT start, intT exclude) {
  Bag_reducer<intT> *queue[2];
  Bag_reducer<intT> b1;
  Bag_reducer<intT> b2;
  queue[0] = &b1;
  queue[1] = &b2;

  excludeNode = exclude;
  queue[0]->insert(start);
  bool queuei = 0;

  reducer_opadd<ll> addWork;
  intT d = nodes[start].h;
  while (!queue[queuei]->isEmpty()) {
    intT sz = queue[queuei]->numElements();
    //cout << "level " << d << " nodes " << sz << endl;
#if GAP_HEURISTIC
    gap = d + 1;
    labelCounter[d] = sz;
#endif
    queue[!queuei]->clear();
    pbfs_walk_Bag(&queue[queuei]->get_reference(),
                  queue[!queuei], addWork);
    queuei = !queuei;
    ++d;
  }
  if (stats) globRelWork += addWork.get_value();
}

#elif BFS_METHOD == 4

#error "Not implemented"
void revBfs(intT start, intT exclude) {
  ass(bfsFrontier.newNodeSetSz == 0);
  bfsFrontier.newNodeSet[bfsFrontier.newNodeSetSz++] = start;
  bfsFrontier.switchSets();
  intT d = nodes[start].h;
  while (!bfsFrontier.empty()) {
    intT *curNodeSet = bfsFrontier.curNodeSet;
    intT sz = bfsFrontier.curNodeSetSz;
    //integerSort<uintT>(curNodeSetSz, sz);
    //TODO remove duplicates and update sz
    //cout << "level " << d << " nodes " << sz << endl;
#if GAP_HEURISTIC
    gap = d + 1;
    labelCounter[d] = sz;
#endif
    parallel_for (intT i = 0; i < sz; ++i) {
      intT wi = curNodeSet[i];
      Node& w = nodes[wi];
      ass(w.lastRelabelTime >= 0);
      intT *out = w.buf;
      for_arcs(for, w, ar, {
        if (!ar.revResCap) continue;
        intT vi = ar.to;
        if (vi == exclude) continue;
        Node& v = nodes[vi];
        if (v.h == hiLabel) {
          v.h = w.h + 1;
          bfsFrontier.addNode_(vi, out);
        }
      })
      bfsFrontier.reportEnd(i, out);
    }
    bfsFrontier.pack();
    bfsFrontier.switchSets();
    d++;
  }
}

#else
#  error "You must define BFS_METHOD = {1,2,3}"
#endif

void globalRelabel() {
  cout << "global relabeling... " << flush;
  workSinceUpdate = 0;
  globalRelabelTime.start();
  globalRelabels++;

  parallel_for (intT i = 0; i < n; ++i) {
    nodes[i].lastRelabelTime = pass;
    nodes[i].h = hiLabel;
    nodes[i].cur = nodes[i].first;
    labelCounter[i] = 0;
  }
  lastGapTime = pass;

  nodes[sink].h = 0;
  revBfs(sink, source);
  nodes[source].h = n;
  if (!minCutOnly) {
    revBfs(source, -1);
  }

  globalRelabelTime.stop();
  cout << "done." << endl;
}

void printStats() {
  cout << setprecision(4) << fixed;
  cout << "after " << pass << " iterations:" << endl
       << "  #nodes        " << setw(20) << n << endl
       << "  #edges        " << setw(20) << m / 2 << endl
       << "  #active nodes " << setw(20) << prFrontier.curNodeSetSz << endl
       << "  #relabels     " << setw(20) << relabels << endl
       << "  rl / dc       " << setw(20) << (double)relabels / totActiveNodes << endl
       << "  #pushes       " << setw(20) << pushes << endl
       << "  #p + #r       " << setw(20) << pushes + relabels << endl
       << "  #glrel        " << setw(20) << globalRelabels << endl
       << "  #gaps         " << setw(20) << gaps << endl
       << "  #gap nodes    " << setw(20) << removedByGap << endl
       << "  work glrel    " << setw(20) << globRelWork << endl
       << "  total work    " << setw(20) << globRelWork + otherWork << endl
       << endl
       << "  total time    " << setw(20) << totalTime.total() << endl
       << "  glrel time    " << setw(20) << globalRelabelTime.total() << endl
       << "  phase1 time   " << setw(20) << timePhase1.total() << endl
       << "  phase2 time   " << setw(20) << timePhase2.total() << endl
       << "  phase3 time   " << setw(20) << timePhase3.total() << endl
       << "  gap heur time " << setw(20) << gapHeuristicTime.total() << endl
       << endl
       << "  sink excess   " << setw(20) << totalExcess(nodes[sink]) << endl
       ;
  if (pass)
       cout << "  avg nodeset   " << setw(13) << (totActiveNodes / pass) << endl;
  cout << endl;
}

void increaseArcFlow(Arc& a, intT delta) {
  a.resCap -= delta;
  Arc& rev = arcs[a.rev];
  rev.revResCap = a.resCap;
  rev.resCap += delta;
  a.revResCap = rev.resCap;
}

void run() {
  pass = 0;
  gaps = 0;
  totActiveNodes = 0;
  globalRelabels = 0;
  globalRelabelTime.clear();
  lastGapTime = 0;
  globRelWork = otherWork = 0;
  removedByGap = 0;
  totalTime.clear();
  gapHeuristicTime.clear();
  timePhase1.clear();
  timePhase2.clear();
  timePhase3.clear();
  totalTime.start();

  hiLabel = minCutOnly ? n : 2*n;
  gap = hiLabel;
  Node& S = nodes[source];
  S.h = n;
  parallel_for (intT i = 0; i < outDegree(S); ++i) {
    Arc& a = arcs[S.first + i];
    prFrontier.curNodeSet[i] = a.to;
    intT delta = a.resCap;
    if (!delta) continue;
    increaseArcFlow(a, delta);
#if DELAY_EXCESS_INCREASE
    nodes[a.to].eadd += delta;
#else
    nodes[a.to].e += delta;
#endif
  }
  prFrontier.newNodeSetSz =
      sequence::filter(prFrontier.curNodeSet, prFrontier.newNodeSet, outDegree(S),
                            [&](intT i) { return totalExcess(nodes[i]) > 0; });
  prFrontier.newNodeSetSz = unique(prFrontier.newNodeSet,
                                   prFrontier.newNodeSet + prFrontier.newNodeSetSz)
                              - prFrontier.newNodeSet;
  pushes += outDegree(S);

  prFrontier.switchSets();
  relabels = pushes = 0;
#if INITIAL_RELABEL
  workSinceUpdate = numeric_limits<decltype(workSinceUpdate)>::max();
#else
  workSinceUpdate = 0;
#endif
  ll nm = ALPHA * (ll)n + (ll)m/2;
  bool sequential = 0;
  int oldNumThreads = getWorkers();
  while (!prFrontier.empty()) {
    intT sz = prFrontier.curNodeSetSz;
    intT *curNodeSet = prFrontier.curNodeSet;
    //cout << "iteration " << pass << " active " << sz << endl;
    //cout << "  active="; for (intT i= 0; i < sz; ++i) cout << setw(3) << curNodeSet[i] << " ";cout << endl;
    //cout << "  e=     "; for (intT vi = 0; vi < n; ++vi) cout << setw(3) << totalExcess(nodes[vi]) << " ";cout << endl;
    //cout << "  h=     "; for (intT vi = 0; vi < n; ++vi) cout << setw(3) << nodes[vi].h << " ";cout << endl;
    if (pass < 50 || statsInterval > 0 && pass % statsInterval == 0)
      printStats();

    if (sz < 50 && !sequential) {
      sequential = 1;
      cout<< "Falling back to sequential after " << totalTime.total() << endl;
      setWorkers(1);
    } else if (sz > 50 && sequential) {
      sequential = 0;
      cout<< "Back to parallel after " << totalTime.total() << endl;
      setWorkers(oldNumThreads);
    }

#if DELAY_EXCESS_INCREASE
    // phase 3
    timePhase3.start();
    parallel_for (intT i = 0; i < sz; ++i) {
      Node& v = nodes[curNodeSet[i]];
      //if (v.eadd) cout << "add " << curNodeSet[i] << " " << v.e
      //                 << " " << v.eadd << endl;
      v.e += v.eadd;
      v.eadd = 0;
    }
    timePhase3.stop();
#endif

    totActiveNodes += sz;
    pass++;

    // randLocal_5_1M: use 1e7
    // randLocal_5_10M: use 20e7
    // randLocal_5_200K: use 5e5
    // rMat_5_200K: use 1e6
    // rMat_5_1M: use 1e6
    //if (relabels + pushes >= workLastGlobalRelabel + 1e7) {
    //if (globalRelabelTime.total() < totalTime.total() / 4) {
    if (workSinceUpdate * globUpdtFreq > nm) {
      globalRelabel();
#if !DELAY_EXCESS_INCREASE && SORT_DESC_BY_LABEL
      // TODO use sort by instead of extra array
      parallel_for (intT i = 0; i < sz; ++i) {
        intT vi = curNodeSet[i];
        Node& v = nodes[vi];
        ass(v.h <= hiLabel);
        ass(v.h >= 0);
        nodesByLabel[i] = { hiLabel - v.h, vi };
      }
      sortByFstInts(nodesByLabel, sz, hiLabel);
      parallel_for (intT i = 0; i < sz; ++i) curNodeSet[i] = nodesByLabel[i].second;
#endif
    }

    //debugGraph();
    //debugState();
    //checkLabelCounter();
    //checkSanity();

    timePhase1.start();

//#define HIGHEST_LABEL
#if HIGHEST_LABEL
    static intT *tmp;
    if (!tmp) tmp = new intT[n];
    intT tmpSz = 0;
//#define NODES_PER_ITER 20
    //if (sz > NODES_PER_ITER) {
    if (sz > 50) {
      intT NODES_PER_ITER = sz/3;
      intSort::iSort(curNodeSet, sz, n + 1,
           [&](intT i) { return hiLabel - nodes[i].h; });
      parallel_for (intT i = NODES_PER_ITER; i < sz; ++i) {
        sizes[i] = 0;
        ignoreNode[i] = 0;
        prFrontier.inNewNodeSet[curNodeSet[i]].test_and_set();
        tmp[i - NODES_PER_ITER] = curNodeSet[i];
      }
      tmpSz = sz - NODES_PER_ITER;
      sz = NODES_PER_ITER;
    }
#endif

    // phase 1
    //cout << "phase 1" << endl;
#if !OPENMP
    cilk::reducer_opadd<ll> addPushes, addWork;
#endif
    // TODO pack between phases using ignoreNoe
    parallel_for (intT i = 0; i < sz; ++i) {
      // TODO can we process edges in parallel?
      intT vi = curNodeSet[i];
      Node& v = nodes[vi];
      sizes[i] = 0; // THIS MUST BE EXECUTED FOR EVERY NODE (EVEN IRRELEVANT)
      ignoreNode[i] = 0;
#if DELAY_EXCESS_INCREASE
      ass2(v.e, pass);
      if (!nodeRelevant(v)) {
#else
      if (!nodeRelevant(v) || !v.e) {
        // v.e can be zero if v was activated but passed its flow through to a neighbor
#endif
        ignoreNode[i] = 1;
        continue;
      }
      intT *out = v.buf;
      //for (intT ai = v.first; ai < v.cur; ++ai) {
        //Arc& a = arcs[ai];
        //ass(!a.resCap || v.h != nodes[a.to].h + 1);
      //}
#if !ADMISSIBLE_EDGE_DS
      v.cur = v.first;
#endif
      while (v.e > 0 && v.cur < last(v)) {
#if !OPENMP
        if (stats) ++addWork;
#endif
        Arc& cur = arcs[v.cur];
        intT c = cur.resCap;
        intT wi = cur.to;
        Node& w = nodes[wi];
        if (!c || v.h != w.h + 1) { // edge already inadmissible
          v.cur++;
          continue;
        }
        intT delta = min(c, (intT)v.e);
        ass(delta);
#if !OPENMP
        if (stats) ++addPushes;
#endif
        //cout << "push " << vi << " " << wi << " " << delta << endl;
        v.e -= delta;
#if DELAY_EXCESS_INCREASE
        w.eadd += delta;
#else
        w.e += delta;
#endif
        increaseArcFlow(cur, delta);
        if (wi != source && wi != sink)
          prFrontier.addNode(wi, out);
        if (delta == c)
          ++v.cur; // if push was saturating, edge is now inadmissible
      }
      if (v.e > 0) {
        //cout << vi << " still active" << endl;
        prFrontier.addNode(vi, out);
        //ass(v.cur == v.last);
      }
      prFrontier.reportEnd(i, out);
    }

    //parallel_for (intT i = 0; i < sz; ++i) {
      //Node& v = nodes[curNodeSet[i]];
      //if (!v.e) continue;
      //for (intT ai = v.first; ai < v.cur; ++ai) {
        //Arc& a = arcs[ai];
        //ass(!admissable(v, a));
      //}
      //if (v.e > 0) ass(v.cur == v.last);
    //}

    prFrontier.pack();
#if HIGHEST_LABEL
    parallel_for (intT i = 0; i < tmpSz; ++i)
      prFrontier.newNodeSet[prFrontier.newNodeSetSz + i] = tmp[i];
    prFrontier.newNodeSetSz += tmpSz;
#endif

#if !OPENMP
    if (stats) pushes += addPushes.get_value();
#endif
    timePhase1.stop();

    // phase 2.1 / compute new labels
    //cout << "phase 2" << endl;
    timePhase2.start();
    ll *addWorkSinceUpdate = new ll[sz];
    int *addRelabels = new int[sz];
    parallel_for (intT i = 0; i < sz; ++i) {
      addWorkSinceUpdate[i] = 0;
      addRelabels[i] = 0;
      Node& v = nodes[curNodeSet[i]];
      if (!minCutOnly)
        ass(!v.e || v.h < hiLabel);
      if (!v.e) ignoreNode[i] = 1;
      if (ignoreNode[i]) {
        newLabel[i] = hiLabel;
#if GAP_HEURISTIC && LABEL_COUNTER_METHOD == 2
        oldLabel[i] = hiLabel;
#endif
        continue;
      }
#if GAP_HEURISTIC && LABEL_COUNTER_METHOD == 2
      oldLabel[i] = v.h;
      ass2(oldLabel[i] < hiLabel, v.h << " " << gap << " " << v.lastRelabelTime << " " << lastGapTime);
#endif
      // TODO use CAS-based writeMin
      //if (outDegree(v) < 1000) {
        intT newh = hiLabel;
        for_arcs(for, v, a, {
          if (a.resCap)
            newh = min(newh, nodes[a.to].h + 1);
        })
        newLabel[i] = newh;
      //} else {
        //cilk::reducer_min<intT> newh;
        //newh.calc_min(hiLabel);
        //for_arcs(parallel_for, v, a, {
          //if (a.resCap)
            //newh.calc_min(nodes[a.to].h + 1);
        //})
        //newLabel[i] = newh.get_value();
      //}
      //ass2(newLabel[i] < hiLabel, oldLabel[i] << " " << newLabel[i]);
#if !DELAY_EXCESS_INCREASE
      if (newLabel[i] == v.h) {
        // this can happen if a node has terminated because of excess zero and
        // got pushed to from another node after termination
        newLabel[i] = hiLabel;
#if GAP_HEURISTIC && LABEL_COUNTER_METHOD == 2
        oldLabel[i] = hiLabel;
#endif
        ignoreNode[i] = 1;
        continue;
      }
#endif
#if !OPENMP
      if (stats) {
        addWork += outDegree(v);
      }
#endif
      addWorkSinceUpdate[i] += BETA + outDegree(v);
      addRelabels[i] = 1;
      v.cur = v.first;
    }
    workSinceUpdate += sequence::plusReduce(addWorkSinceUpdate, sz);
    relabels += sequence::plusReduce(addRelabels, sz);
#if !OPENMP
    if (stats) otherWork += addWork.get_value();
#endif

    // phase 2.2 / apply new labels + gap heuristics
#if GAP_HEURISTIC
    cilk::reducer_min<intT> newGap;
    newGap.calc_min(inf);
#endif

    parallel_for (intT i = 0; i < sz; ++i) {
      Node& v = nodes[curNodeSet[i]];
      if (ignoreNode[i]) continue;
#if LABEL_COUNTER_METHOD == 1 and GAP_HEURISTIC
      --labelCounter[v.h];
      ++labelCounter[newLabel[i]];
      if (!labelCounter[v.h])
        newGap.calc_min(v.h);
#endif
      ass2(newLabel[i] > v.h, (&v - nodes) << " " << v.e << " "
                              << newLabel[i] << " " << v.h << " "
                              << pass << " " << v.lastRelabelTime << " "
                              << lastGapTime << " " << gap);
      //ass(newLabel[i] < hiLabel);
      //cout << "relabel " << (&v-nodes) << " " << v.h << " -> " << newLabel[i] << endl;
      v.h = newLabel[i];
      v.lastRelabelTime = pass;
    }

#if GAP_HEURISTIC
#  if LABEL_COUNTER_METHOD == 2
    static_assert(!OPENMP);
    intT groups = groupInts(oldLabel, firstTmp, first, sz, hiLabel);
    if (oldLabel[first[groups - 1]] == hiLabel) groups--;
    parallel_for (intT i = 0; i < groups; ++i) {
      intT label = oldLabel[first[i]];
      ass(label < hiLabel);
      intT cnt = first[i + 1] - first[i];
      if (cnt == labelCounter[label])
        newGap.calc_min(label);
      labelCounter[label] -= cnt;
    }
    groups = groupInts(newLabel, firstTmp, first, sz, hiLabel);
    if (newLabel[first[groups - 1]] == hiLabel) groups--;
    parallel_for (intT i = 0; i < groups; ++i) {
      intT label = newLabel[first[i]];
      ass(label < hiLabel);
      intT cnt = first[i + 1] - first[i];
      labelCounter[label] += cnt;
    }
#  endif
    if (newGap.get_value() < inf) {
      cout << "gap detected: " << newGap.get_value() << endl;
      ++gaps;
      lastGapTime = pass;
      gap = newGap.get_value();
      gapHeuristicTime.start();
      reducer_opadd<intT> addRemoved;
      parallel_for (intT i = 0; i < n; ++i) {
        if (nodes[i].h > gap) {
          if (stats) ++addRemoved;
          nodes[i].h = n;
        }
      }
      if (stats) removedByGap += addRemoved.get_value();
      parallel_for (intT i = gap + 1; i < n; ++i)
        labelCounter[i] = 0;
      gapHeuristicTime.stop();
    }
#endif
    timePhase2.stop();

    prFrontier.switchSets();
  }
  //checkSanity();
  //parallel_for (intT i = 0; i < n; ++i) {
    //if (i != sink && i != source && nodeRelevant(nodes[i])) ass(!nodes[i].e);
  //}
  printStats();
  cout << "Resetting number of threads to " << oldNumThreads << endl;
  setWorkers(oldNumThreads);
}

intT maxFlow(FlowGraph<intT>& g) {
  parallel_for (intT i = 1; i < n; ++i) {
    ass(g.g.V[i].Neighbors == g.g.V[i - 1].Neighbors + g.g.V[i - 1].degree);
    ass(g.g.V[i].nghWeights == g.g.V[i - 1].nghWeights + g.g.V[i - 1].degree);
  }
  intT *adj = g.g.V[0].Neighbors, *weights = g.g.V[0].nghWeights;

  timer timeInit; timeInit.start();
  n = g.g.n, m = 2 * g.g.m;
  nodes = new Node[n + 1];
  source = g.source;
  sink = g.sink;
  cout << "source=" << source << " sink=" << sink << endl;

  timer t; t.start();
  nodes[n].first = m;
  intT *cap = new intT[m / 2];
  intT *edges = new intT[m];
  parallel_for (intT i = 0; i < n; ++i) {
    nodes[i].first = -1;
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
  COMP_SORT(edges, edges + m, [&] (intT x, intT y) { return toEdge(x) < toEdge(y); });

  intT *firstTmp = new intT[m], *first = new intT[m];
  firstTmp[0] = 0;
  parallel_for (intT i = 1; i < m; ++i)
    firstTmp[i] = (toEdge(edges[i]).first != toEdge(edges[i - 1]).first) ? i : -1;
  intT sz = sequence::filter(firstTmp, first, m, nonNegF<intT>);
  parallel_for (intT i = 0; i < sz; ++i)
    nodes[toEdge(edges[first[i]]).first].first = first[i];
  t.stop(); cout << "t2 = " << t.total() << endl; t.clear(); t.start();
  delete[] firstTmp;
  delete[] first;

  for (intT i = n - 1; i >= 0; --i)
    if (nodes[i].first < 0) nodes[i].first = nodes[i + 1].first;
  t.stop(); cout << "t3 = " << t.total() << endl; t.clear(); t.start();

  intT* pos = new intT[m];
  parallel_for (intT i = 0; i < m; ++i)
    pos[edges[i]] = i;
  arcs = new Arc[m];
  t.stop(); cout << "t4 = " << t.total() << endl; t.clear(); t.start();
  parallel_for (intT i = 0; i < m; ++i) {
    intT idx = edges[i];
    pair<intT, intT> fromTo = toEdge(idx);
    //arcs[i].isorig = !(idx & 1);
    arcs[i].rev = pos[idx ^ 1];
    arcs[i].to = fromTo.second;
    arcs[i].resCap = (idx & 1) ? 0 : cap[idx / 2];
  }
  delete[] pos;
  delete[] cap;
  t.stop(); cout << "t5 = " << t.total() << endl; t.clear(); t.start();

  parallel_for (intT i = 0; i < m; ++i) {
    intT pairId = edges[i] / 2;
    if (edges[i] & 1) weights[pairId] = i;
  }
  delete[] edges;

  init();
  t.stop(); cout << "t6 = " << t.total() << endl; t.clear(); t.start();
  timeInit.stop();
  cout << "init time: " << timeInit.total() << endl;
  beforeHook();
  run();
  afterHook();
  timer timeDeinit; timeDeinit.start();
  deinit();

  atomic<intT> flow = 0;
  parallel_for (intT i = 0; i < n; ++i) {
    FlowVertex& v = g.g.V[i];
    parallel_for (intT j = 0; j < v.degree; ++j) {
      v.nghWeights[j] = arcs[v.nghWeights[j]].resCap;
      if (v.Neighbors[j] == sink) flow += v.nghWeights[j];
    }
  }
  ass(flow == totalExcess(nodes[sink]));
  cout << "flow=" << flow << endl;
  delete[] nodes;
  delete[] arcs;
  timeDeinit.stop();
  cout << "deinit time: " << timeDeinit.total() << endl;
  return flow;
}
