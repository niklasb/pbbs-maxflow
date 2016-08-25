// config
const bool stats = 1;
const int statsInterval = 5;
#define INITIAL_RELABEL

const float globUpdtFreq = 0.5;
#define ALPHA 6
#define BETA 12

//#ifdef SEQUENTIAL
////#  undef CILK
////#  undef CILKP
////#  undef OPENMP
//#  include "atomic_nop.h"
//#else
//#  include "atomic_x86.h"
////#  include <atomic>
//#endif

#include <iostream>
#include <queue>
#include <set>

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <cilk/reducer_opadd.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_ostream.h>

#include "tbb/concurrent_priority_queue.h"
#include "tbb/task_group.h"
#include "tbb/task_scheduler_init.h"

#include <unistd.h>

#include "graph.h"
#include "gettime.h"
#include "utils.h"
#include "sequence.h"
#include "parallel.h"
#include "maxFlow.h"
#include "bag.h"
#include "blockRadixSort.h"
#include "prioQueue.h"
using namespace std;
using namespace cilk;
using namespace tbb;
typedef long long ll;

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

template<typename T> bool nonNegF(T x) { return x >= 0; }

struct Arc {
  intT resCap;
  intT to;
  intT rev;
  intT revResCap;
  mutex mx;
} *arcs;

struct Node {
  intT first, cur;
  atomic<intT> e;
  atomic<intT> h;
  mutex mx;
} *nodes;

intT last(Node& v) { return (&v + 1)->first; }
intT outDegree(Node& v) { return last(v) - v.first; }

mutex& arc_mutex(intT i) {
  if (i < arcs[i].rev) return arcs[i].mx;
  else return arcs[arcs[i].rev].mx;
}

#define for_arcs(for, n, a, body) \
  {for (intT _i = (n).first; _i < last(n); ++_i) { Arc& a = arcs[_i]; body }}

ll nm;
ll pass;
intT n, m, source, sink;
atomic<intT> *labelCounter;
atomic<ll> relabels, pushes;
ll globalRelabels;
atomic<ll> workSinceUpdate;
timer globalRelabelTime, totalTime, gapHeuristicTime;
intT gaps;
intT lastGapTime;
atomic<ll> globRelWork;
atomic<ll> otherWork;
intT removedByGap;
intT hiLabel;
SharedPrioQueue<pair<int,int> > q;

void init() {
  // n, m, source, sink and the graph structure stored in nodes, arcs must be
  // initialized already
  q.reset();
  labelCounter = new atomic<intT>[n];
  parallel_for (intT i = 0; i < n; ++i) labelCounter[i] = 0;
  labelCounter[0] = n - 1;
  for (intT i = 0; i < n; ++i) {
    nodes[i].h = 0;
    nodes[i].e = 0;
    nodes[i].cur = nodes[i].first;
  }
  parallel_for (intT i = 0; i < m; ++i) {
    arcs[i].revResCap = arcs[arcs[i].rev].resCap;
  }
}

void deinit() {
  delete[] labelCounter;
}

void activate(int v) {
  q.enqueue(make_pair(nodes[v].h.load(), v));
}
bool get_node(int& v) {
  pair<int,int> x;
  while (q.dequeue(x)) {
    //cout << "get_node " << x.second << " "<< x.first << " "<< nodes[x.second].h << endl;
    if (nodes[x.second].h == x.first) {
      v = x.second;
      return 1;
    }
  }
  return 0;
}
bool have_nodes() {
  return !q.empty();
}

void checkLabelCounter() {
  static atomic<intT> *lblCnt;
  if (!lblCnt) lblCnt = new atomic<intT>[2 * n + 1];
  parallel_for (intT i = 0; i < hiLabel; ++i) lblCnt[i].store(labelCounter[i]);
  parallel_for (intT i = 0; i < n; ++i) lblCnt[nodes[i].h]--;
  parallel_for (intT i = 0; i < hiLabel; ++i) ass(!lblCnt[i]);
}

void checkDistanceLabels() {
  parallel_for (intT i = 0; i < n; ++i) {
    if (nodes[i].h == hiLabel) continue;
    for_arcs(parallel_for, nodes[i], a, {
      if (a.resCap > 0) {
        ass2(nodes[i].h <= nodes[a.to].h + 1,
               i << " " << a.to << " " << nodes[i].h << " " << nodes[a.to].h);
      }
    })
  }
}

void checkSanity() {
  ass(nodes[source].h == n);
  ass(nodes[sink].h == 0);
  checkDistanceLabels();
  checkLabelCounter();
  parallel_for (intT i = 0; i < n; ++i) {
    Node& v = nodes[i];
    for_arcs(parallel_for, v, a, {
      ass(a.resCap == arcs[a.rev].revResCap);
      ass(a.revResCap == arcs[a.rev].resCap);
      ass(a.resCap >= 0);
      ass(arcs[a.rev].resCap >= 0);
    })
  }
}

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
    if (stats) work += outDegree(w);
    for_arcs(for, w, ar, {
      if (!ar.revResCap) continue;
      intT vi = ar.to;
      if (vi == excludeNode) continue;
      Node& v = nodes[vi];
      intT hi = hiLabel;
      if (v.h.compare_exchange_strong(hi, w.h + 1)) {
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
    labelCounter[d] = sz;
    queue[!queuei]->clear();
    pbfs_walk_Bag(&queue[queuei]->get_reference(),
                  queue[!queuei], addWork);
    queuei = !queuei;
    ++d;
  }
  if (stats) globRelWork += addWork.get_value();
}

void globalRelabel() {
  cout << "global relabeling " << endl;
  //cout << "hiLabel=" << hiLabel <<  endl;
  q.reset();
  workSinceUpdate.store(0);
  if (stats) globalRelabelTime.start();
  if (stats) globalRelabels++;

  parallel_for (intT i = 0; i < n; ++i) {
    nodes[i].h = hiLabel;
    nodes[i].cur = nodes[i].first;
    labelCounter[i] = 0;
  }

  nodes[sink].h = 0;
  revBfs(sink, source);
  nodes[source].h = n;
  for (intT i = 0; i < n; ++i) {
    if (nodes[i].e && nodes[i].h < hiLabel) activate(i);
  }
  if (stats) globalRelabelTime.stop();
  cout << "done" << endl;
}

void printStats() {
  cout << setprecision(4) << fixed;
  cout << "after " << pass << " iterations:" << endl
       << "  #nodes        " << setw(20) << n << endl
       << "  #edges        " << setw(20) << m / 2 << endl
       //<< "  #active nodes " << setw(20) << prFrontier.curNodeSetSz << endl
       << "  #relabels     " << setw(20) << relabels << endl
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
       //<< "  phase1 time   " << setw(20) << timePhase1.total() << endl
       //<< "  phase2 time   " << setw(20) << timePhase2.total() << endl
       //<< "  phase3 time   " << setw(20) << timePhase3.total() << endl
       << "  gap heur time " << setw(20) << gapHeuristicTime.total() << endl
       << endl
       << "  sink excess   " << setw(20) << nodes[sink].e << endl
       ;
  //if (pass)
       //cout << "  avg nodeset   " << setw(13) << (totActiveNodes / pass) << endl;
  cout << endl;
}

void increaseArcFlow(Arc& a, intT delta) {
  a.resCap -= delta;
  Arc& rev = arcs[a.rev];
  rev.revResCap = a.resCap;
  rev.resCap += delta;
  a.revResCap = rev.resCap;
}

bool needGlobalUpdate() {
  return (workSinceUpdate * globUpdtFreq > nm);
}

void worker(int i, int maxIterations = 10000) {
  int vi;
  //cout << "hi from worker " << i << endl;
  while (maxIterations-- && !needGlobalUpdate() && get_node(vi)) {
    //cout << i << " got node " << vi << " iters left = " << iterations <<  endl;
    ll addOtherWork = 0, addWorkSinceUpdate = 0,
       addRelabels = 0, addPushes = 0;
    Node& v = nodes[vi];
    {
      unique_lock<mutex> node_lock(v.mx);
      for(;;) {
        while (v.e && v.cur < last(v)) {
          if (stats) ++addOtherWork;
          Arc& cur = arcs[v.cur];
          intT c = cur.resCap;
          intT wi = cur.to;
          Node& w = nodes[wi];
          if (!c ||  v.h != w.h + 1) {
            v.cur++;
            continue;
          }
          {
            unique_lock<mutex> lock(arc_mutex(v.cur));
            c = cur.resCap;
            if (v.h != w.h + 1) {
              v.cur++;
              continue;
            }
            intT delta = min(c, (intT)v.e);
            ass(delta);
            //cout << "push " << vi << " " << wi << " " << delta << endl;
            if (stats) ++addPushes;
            v.e -= delta;
            w.e += delta;
            increaseArcFlow(cur, delta);
            if (wi != source && wi != sink)
              activate(wi);
            if (delta == c) ++v.cur;
          }
        }
        if (!v.e) break;
        intT newh = hiLabel;
        addWorkSinceUpdate += BETA;
        for_arcs(for, v, a, {
          addWorkSinceUpdate++;
          if (a.resCap)
            newh = min(newh, nodes[a.to].h + 1);
        });
        if (stats) addRelabels++;
        //cout << "relabel " << vi << " " << v.h << " -> " << newh << endl;
        v.h = newh;
        v.cur = v.first;
        if (v.h == hiLabel) break;
      }
    }
    workSinceUpdate += addWorkSinceUpdate;
    if (stats) {
      otherWork += addOtherWork;
      relabels += addRelabels;
      pushes += addPushes;
    }
    //cout << "done " << vi << endl;
  }
}

void run() {
  if (stats) {
    pass = 0;
    gaps = 0;
    globalRelabels = 0;
    globalRelabelTime.clear();
    lastGapTime = 0;
    globRelWork = otherWork = 0;
    removedByGap = 0;
    totalTime.clear();
    gapHeuristicTime.clear();
    totalTime.start();
    globalRelabelTime.clear();
    pushes.store(0);
    relabels.store(0);
  }
  hiLabel = n;
  Node& S = nodes[source];
  S.h = n;
  parallel_for (intT i = 0; i < outDegree(S); ++i) {
    Arc& a = arcs[S.first + i];
    intT delta = a.resCap;
    increaseArcFlow(a, delta);
    nodes[a.to].e += delta;
    //cout << "activating " << a.to << endl;
    activate(a.to);
  }
  pushes += outDegree(S);
#ifdef INITIAL_RELABEL
  workSinceUpdate.store(numeric_limits<ll>::max());
#else
  workSinceUpdate.store(0);
#endif
  int nworkers = getWorkers();
  if (nworkers < 0) nworkers = 16;
  nworkers = 32;
  vector<thread> threads;
  cout << "Running with " << nworkers << " threads" << endl;
  nm = ALPHA * (ll)n + (ll)m/2;
  atomic<int> finished(0);
  q.should_stop = [&](int waiting) {
    //cout << "waiting + finished / nworkers = " << waiting+finished << " / " << nworkers << endl;
    return waiting + finished >= nworkers;
  };
  while (have_nodes()) {
    ++pass;
    cout << "iteration " << pass << " " << workSinceUpdate << endl;
    if (stats && pass % statsInterval == 0) printStats();
    if (needGlobalUpdate())
      globalRelabel();
    threads.clear();
    finished.store(0);
    q.restart();
    for (int i = 0; i < nworkers; ++i)
      threads.emplace_back([=,&finished](){
          worker(i);
          finished++;
          if (q.waiting + finished >= nworkers) q.stop();
      });
    for (thread& t: threads) t.join();
    assert(q.waiting == 0);
  }
  printStats();
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
  static_assert(sizeof(ll) >= 2 * sizeof(intT), "Need to be able to square");
  intSort::iSort(edges, m, (ll)(n - 1) * (n + 1) + 1,
        [&] (intT x) {
          pair<intT,intT> fromTo = toEdge(x);
          return (ll)fromTo.first * n + fromTo.second;
        });

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

  atomic<intT> flow(0);
  parallel_for (intT i = 0; i < n; ++i) {
    FlowVertex& v = g.g.V[i];
    parallel_for (intT j = 0; j < v.degree; ++j) {
      v.nghWeights[j] = arcs[v.nghWeights[j]].resCap;
      if (v.Neighbors[j] == sink) flow += v.nghWeights[j];
    }
  }
  ass(flow == nodes[sink].e);
  cout << "flow=" << flow << endl;
  delete[] nodes;
  delete[] arcs;
  timeDeinit.stop();
  cout << "deinit time: " << timeDeinit.total() << endl;
  return flow;
}
