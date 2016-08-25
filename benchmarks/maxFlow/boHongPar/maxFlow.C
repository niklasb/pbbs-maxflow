// config
#include <iostream>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include "graph.h"
#include "gettime.h"
#include "utils.h"
#include "sequence.h"
#include "parallel.h"
#include "maxFlow.h"
#include "bag.h"
#include "blockRadixSort.h"
#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/task_group.h"
#include <parallel/algorithm>
#include <unistd.h>

#define STATS 0 // disable for performance

#define AMORTIZATION_SCHEME 2 // 1 = hipr
                              // 2 = bo-hong

#define METHOD 1         // 1 = original version
                         // 2 = only find first lower vertex for push
                         // 3 = push while scanning

#define COMP_SORT __gnu_parallel::sort
using namespace std;
typedef long long ll;

using Cap = long long;
mutex iomutex;
#define ATOMIC(x) { iomutex.lock(); x iomutex.unlock(); }
//#define TRACE(x) ATOMIC(x)
#define TRACE(x)

template<typename T> bool nonNegF(T x) { return x >= 0; }

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

template<typename F>
class lambda_task : public tbb::task {
  F my_func;
  /*override*/ tbb::task* execute() {
    my_func();
    return NULL;
  }
public:
  lambda_task( const F& f ) : my_func(f) {}
};
template<typename F>
void tbb_enqueue_lambda( const F& f ) {
  tbb::task::enqueue( *new( tbb::task::allocate_root() ) lambda_task<F>(f) );
}

struct Arc {
  atomic<Cap> resCap, revResCap;
  intT to;
  intT rev;
} *arcs;

struct Node {
  intT first;
  atomic<intT> h;
  atomic<Cap> e;
  intT wave;
  atomic_flag queued;
  //atomic_flag processing;
} *nodes;

#if AMORTIZATION_SCHEME == 1
  const float globUpdtFreq = 1000;
#else
  const float globUpdtFreq = 1;
#endif
const int alpha = 6, beta = 12;
#define INITIAL_RELABEL 1

intT last(Node& v) { return (&v + 1)->first; }
intT outDegree(Node& v) { return last(v) - v.first; }

#define for_arcs(for, n, a, body) \
  {for (intT _i = (n).first; _i < last(n); ++_i) { Arc& a = arcs[_i]; body }}

intT n, m, source, sink, currentWave;
ll nm;
intT* dis;
bool finished;

void init() {
  currentWave = 0;
  // n, m, source, sink and the graph structure stored in nodes, arcs must be
  // initialized already
  for (intT i = 0; i < n; ++i) {
    nodes[i].h = 0;
    nodes[i].e = 0;
    nodes[i].queued.clear();
    nodes[i].wave = 0;
  }
  parallel_for (intT i = 0; i < m; ++i) {
    arcs[i].revResCap.store(arcs[arcs[i].rev].resCap);
  }
  dis = new intT[n];
}

void deinit() { 
  delete[] dis;
}

void debugState() {
  cout << "e = ";
  for (intT i = 0; i < n; ++i) cout << nodes[i].e << " "; cout << endl;
  cout << "h = ";
  for (intT i = 0; i < n; ++i) cout << nodes[i].h << " "; cout << endl;
  //cout << "newh = ";
  //for (intT i = 0; i < n; ++i) cout << nodes[i].newh << " "; cout << endl;
  cout << "active nodes = ";
  for (intT i = 0; i < n; ++i)
    if (nodes[i].e)
      cout << i << " ";
  cout << endl;
}

void checkDistanceLabels() {
  for (intT i = 0; i < n; ++i) {
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
  return;
  parallel_for (intT i = 0; i < n; ++i) {
    Node& v = nodes[i];
    for_arcs(parallel_for, v, a, {
      Node& w = nodes[a.to];
      ass2(a.revResCap == arcs[a.rev].resCap, (&a-arcs) << " " << a.revResCap << " " << arcs[a.rev].resCap);
      ass(a.resCap >= 0);
    })
  }
}

void increaseArcFlow(Arc& a, Cap delta) {
  Arc& rev = arcs[a.rev];
  a.resCap -= delta;
  rev.resCap += delta;
  rev.revResCap.store(a.resCap);
  a.revResCap.store(rev.resCap);
}

struct NodeWorker {
  queue<intT> localQueue;
  atomic<int> exchange;
  int workerId;
  long long localWork;
  NodeWorker(NodeWorker&& w){}
  NodeWorker(intT workerId=0) 
    : exchange(-1), workerId(workerId), localWork(0) {}
  void addWork(long long x);
  void activate(intT ui);
  void process(intT ui);
  void work();
};
vector<NodeWorker> nodeWorkers;
atomic<ll> totalWork, pushes, relabels;
ll workGranularity = 100000;

void NodeWorker::activate(intT ui) {
  if (nodes[ui].queued.test_and_set())
    return;
  //cout << workerId << " pushing " << ui << endl;
  localQueue.push(ui);
}

void NodeWorker::addWork(ll x) {
  localWork+=x;
  if (localWork >= workGranularity) {
    //cout << workerId << " "   << totalWork << "/" << nm << endl;
    totalWork += localWork / workGranularity * workGranularity;
    localWork = localWork % workGranularity;
  }
}

void NodeWorker::process(intT ui) {
#if METHOD==1
  //debugState();
  Node& u = nodes[ui];
  //assert(!u.processing.test_and_set());
  //cout << workerId << " processing " << ui << " " << u.e << " " << u.h << endl;
  while (u.e > 0 && u.h < n) {
    //cout << "   " << ui << endl;
    //cout << "(6.0)u.h = " << u.h << " " << endl;
    Cap ee = u.e;
    intT hh = n - 1;
    intT ai = -1;
    //cout << "inspecting " << outDegree(u) << " arcs" << endl;
    //cout << (void*)(arcs+u.first) << " " << (void*)(arcs+(&u+1)->first) << endl;
    //cout << (void*)&u << endl;
    intT cnt = 0;
    for_arcs(for, u, a, {
      cnt++;
      if (!a.resCap) continue;
      Node& v = nodes[a.to];
      //cout << "checking arc " << a.to << " " << v.h << endl;
      intT vh = v.h;
      if (vh < hh) {
        ai = &a - arcs;
        hh = vh;
      }
    });
    //cout << "inspected " << cnt << endl;
    //cout << "(6.1)u.h = " << u.h << " " << endl;
    //cout << "u.h " << u.h << " hh = " << hh << " ai " << ai << endl;
    if (u.h > hh && u.h < n && ai != -1) {
#if AMORTIZATION_SCHEME == 2
      addWork(1);
#endif
      Arc& a = arcs[ai];
      intT vi = a.to;
      Node& v = nodes[vi];
      Cap delta = min(ee, a.resCap.load());
      //ass2(delta, ee << " " << a.resCap.load());
      //ll nm;
      if (delta && u.wave <= v.wave && u.h > v.h) {
#if STATS
        pushes++;
#endif
        //cout << "(1)u.h = " << u.h << " " << endl;
        //cout << "push " << ui << " -> " << arcs[ai].to << " (" << delta << ")" << endl;
        increaseArcFlow(a, delta);
        //cout << "(2)u.h = " << u.h << " " << endl;
        u.e -= delta;
        v.e += delta;
        //cout << "(3)u.h = " << u.h << " " << endl;
        if (vi != sink && vi != source)
          activate(vi);
        //cout << "(4)u.h = " << u.h << " " << endl;
      }
    } else if (u.h < hh + 1) {
      //cout << "(9)u.h = " << u.h << " " << endl;
#if STATS
      relabels++;
#endif
#if AMORTIZATION_SCHEME == 1
      addWork(outDegree(u) + beta);
#else
      addWork(1);
#endif
      //cout << "relabel " << ui << " to " << (hh+1) << endl;
      u.h = hh + 1;
      //cout << "(10)u.h = " << u.h << " " << endl;
    }
  }
  u.queued.clear();
  //u.processing.clear();
  if (u.e > 0 && u.h < n) activate(ui);
  //cout << "(11)u.h = " << u.h << " " << endl;
  //cout << "finished processing " << ui << endl;
#elif METHOD==2
  //cout << workerId << " processing " << ui << endl;
  Node& u = nodes[ui];
  //assert(!u.processing.test_and_set());
  while (u.e > 0 && u.h < n) {
    //cout << "   " << ui << endl;
    Cap ee = u.e;
    intT hh = n - 1;
    intT ai = -1;
    for_arcs(for, u, a, {
      if (!a.resCap) continue;
      Node& v = nodes[a.to];
      if (v.h < u.h) {
        ai = &a - arcs;
        hh = v.h;
        break;
      }
    });
    if (u.h > hh && u.h < n && ai != -1) {
#if AMORTIZATION_SCHEME == 2
      addWork(1);
#endif
      Arc& a = arcs[ai];
      intT vi = a.to;
      Node& v = nodes[vi];
      Cap delta = min(ee, a.resCap.load());
      //ass2(delta, ee << " " << a.resCap.load());
      //ll nm;
      if (delta && u.wave <= v.wave && u.h > v.h) {
#if STATS
        pushes++;
#endif
        //cout << "push " << ui << " -> " << arcs[ai].to << " (" << delta << ")" << endl;
        increaseArcFlow(a, delta);
        u.e -= delta;
        v.e += delta;
        if (vi != sink && vi != source)
          activate(vi);
      }
    } else {
      hh = n-1;
      for_arcs(for, u, a, {
        if (!a.resCap) continue;
        hh = min(hh, nodes[a.to].h.load());
      });
      if (u.h < hh + 1) {
#if STATS
        relabels++;
#endif
#if AMORTIZATION_SCHEME == 1
        addWork(outDegree(u) + beta);
#else
        addWork(1);
#endif
        //cout << "relabel " << ui << " to " << (hh+1) << endl;
        u.h = hh + 1;
      }
    }
  }
  u.queued.clear();
  //u.processing.clear();
  if (u.e > 0 && u.h < n) activate(ui);
  //cout << "finished processing " << ui << endl;
#elif METHOD==3
  Node& u = nodes[ui];
  //cout << workerId << " processing " << ui << " (e=" << u.e << ", h=" << u.h << ")" << endl;
  //assert(!u.processing.test_and_set());
  while (u.e > 0 && u.h < n) {
    //cout << "   " << ui << endl;
#if AMORTIZATION_SCHEME == 2
    addWork(1);
#endif
    bool retry = 0;
    for_arcs(for, u, a, {
      if (!a.resCap) continue;
      if (!u.e) break;
      intT vi = a.to;
      Node& v = nodes[vi];
      if (v.h < u.h) {
#if STATS
        pushes++;
#endif
        // push
        Cap delta = min(u.e.load(), a.resCap.load());
        if (delta && u.wave <= v.wave && u.h > v.h) {
          //cout << "push " << ui << " -> " << vi << " (" << delta << ")" << endl;
          increaseArcFlow(a, delta);
          u.e -= delta;
          v.e += delta;
          if (vi != sink && vi != source)
            activate(vi);
        } else {
          retry = 1;
        }
      }
    });
    if (u.e && !retry) {
#if STATS
      relabels++;
#endif
      // relabel
#if AMORTIZATION_SCHEME == 1
      addWork(outDegree(u) + beta);
#endif
      intT hh = n;
      for_arcs(for, u, a, {
        if (a.resCap)
          hh = min(hh, nodes[a.to].h + 1);
      });
      if (u.h < hh) {
        //cout << "relabel " << ui << " to " << hh << endl;
        u.h = hh;
      }
    }
  }
  u.queued.clear();
  //u.processing.clear();
  if (u.e > 0 && u.h < n) activate(ui);
  //cout << "finished processing " << ui << endl;
#else
#  error "No such method"
#endif
}

void NodeWorker::work() {
  for(;;) {
check_serve:
    if (exchange.load() != -1) {
      TRACE(cout << workerId << " serving " << exchange.load() << endl;)
      for (intT len = localQueue.size() / 2, i = 0; i < len; ++i) {
        nodeWorkers[exchange].localQueue.push(localQueue.front());
        localQueue.pop();
      }
      nodeWorkers[exchange].exchange.store(-3);
      exchange.store(-1);
      TRACE(cout << workerId << " served" << endl;)
      //cout << "served. exchange = "; for (NodeWorker& w : nodeWorkers) cout << w.exchange.load() << " ";cout << endl;
      //cout << "queue sizes = ";for (NodeWorker& w: nodeWorkers) cout << w.localQueue.size() << " ";cout << endl;
    }
    if (localQueue.empty()) {
      //cout << "Underflow (" << workerId << ")!" << " queues = ";
      //for (int i = 0; i < nodeWorkers.size(); ++i) cout << nodeWorkers[i].localQueue.size() << " ";cout << endl;
      int expected = -1;
      if (!exchange.compare_exchange_strong(expected, -2))
        goto check_serve;
      int i = rand() % nodeWorkers.size();
      int tries = 3*nodeWorkers.size();
      while(localQueue.empty()) {
        //cout << "i = " << i << endl;
        if (i != workerId) {
          NodeWorker& w = nodeWorkers[i];
          //cout << w.localQueue.size() << " " << w.exchange.load() << endl;
          expected = -1;
          assert(exchange.load() == -2);
          if (w.localQueue.size() > 1 && w.exchange.compare_exchange_strong(expected, workerId)) {
            //cout << workerId << " stealing from " << i << endl;
            // spin lock
            TRACE(cout << workerId << " waiting for " << i << endl;)
            while(exchange.load() != -3) {
            }
            exchange.store(-1);
            goto check_serve;
            TRACE(cout << workerId << " done waiting for " << i << endl;)
            //cout << "success! " << localQueue.size() << " " << w.localQueue.size() <<  endl;
            //cout << "exchange = " << exchange.load() << endl;
          }
        }
        i = (i + 1) % nodeWorkers.size();
        tries--;
        if (!tries) break;
      }
      if (!tries) return;
    }
    assert(!localQueue.empty());
    intT ui = localQueue.front();
    localQueue.pop();
    //cout << workerId << " processing " << ui << endl;
    process(ui);
  }
  TRACE(cout << workerId << " exiting " << endl;)
}

void globalRelabel() {
  for(;;) {
    while (!finished && totalWork.load() * globUpdtFreq < nm)  {
      //cout << (totalWork.load()*globUpdtFreq) << " " << nm << endl;
      //sleep(1);
      usleep(50000);
    }
    if (finished) break;
    cout << "globrel" << endl;
    //checkSanity();
    currentWave++;
    fill(dis, dis + n, n);
    queue<intT> q;
    q.push(sink);
    dis[sink] = 0;
    while(!q.empty()) {
      intT ui = q.front();
      Node& u = nodes[ui];
      q.pop();
      for_arcs(for, u, a, {
        if (!a.revResCap) continue;
        intT vi = a.to;
        if (vi == source) continue;
        Node& v = nodes[vi];
        if (dis[vi] > dis[ui] + 1) {
          dis[vi] = dis[ui] + 1;
          if (v.h < dis[vi]) {
            // TODO activate node?
            v.h = dis[vi];
          }
          v.wave = currentWave;
          q.push(vi);
        }
      });
    }
    for (intT i = 0; i < n; ++i) {
      nodes[i].wave = currentWave;
      if (dis[i] == n)
        nodes[i].h = n;
    }
    totalWork = 0;
    cout << "globrel done" << endl;
    //sleep(1);
  }
}

void run() {
  const char* nthreads_c = getenv("OMP_NUM_THREADS");
  if (!nthreads_c) {
    cerr << "Please set OMP_NUM_THREADS!" << endl;
    exit(1);
  }
  int nthreads = atoi(nthreads_c);
#if AMORTIZATION_SCHEME == 1
  nm = (ll)(alpha * (ll)n +  (ll)m / 2);
#else
  nm = n;
#endif
  //workGranularity = max((ll)(nm/globUpdtFreq/10), 1000LL);
#if INITIAL_RELABEL
  totalWork.store(nm);
#else
  totalWork.store(0);
#endif
  pushes.store(0);
  relabels.store(0);
  Node& S = nodes[source];
  S.h = n;
  nodeWorkers.clear();
  nodeWorkers.resize(nthreads);
  // TODO make parallel, but dont push onto queue!!
  for (intT i = 0; i < outDegree(S); ++i) {
    Arc& a = arcs[S.first + i];
    Cap delta = a.resCap;
    if (!delta) continue;
    increaseArcFlow(a, delta);
    nodes[a.to].e += delta;
    //cout << "init push " << source << " -> " << a.to << " (" << delta << ")" << endl;
    if (a.to != sink) {
      nodeWorkers[i % nodeWorkers.size()].localQueue.push(a.to);
      nodes[a.to].queued.test_and_set();
    }
  }
  vector<thread> threads;
  for (int i = 0; i < nthreads; ++i) {
    NodeWorker& w = nodeWorkers[i];
    w.workerId = i;
    threads.emplace_back([&]() {
      w.work();
    });
  }
  thread globRel([]() { globalRelabel(); });
  for (thread& t : threads)
    t.join();
  finished = 1;
  globRel.join();
  cout << "================" << endl;
  cout << "#pushes = " << pushes.load() << endl;
  cout << "#relabels = " << relabels.load() << endl;
  cout << "================" << endl;
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
  cout << "n=" << n << " m=" << m << endl;

  timer t; t.start();
  nodes[n].first = m;
  Cap *cap = new Cap[m / 2];
  intT *edges = new intT[m];
  //intT X = 149390805;
  //cout << "degree 149390805 = " << g.g.V[X].degree << endl;
  //atomic<intT> deg(0);
  //deg += g.g.V[X].degree;
  parallel_for (intT i = 0; i < n; ++i) {
    nodes[i].first = -1;
    FlowVertex& v = g.g.V[i];
    intT offset = v.Neighbors - adj;
    parallel_for (intT j = 0; j < v.degree; ++j) {
      intT pairId = offset + j;
      cap[pairId] = v.nghWeights[j];
      //if (v.Neighbors[j] == X) deg++;
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
  //cout << "deg = " << deg.load() << endl;
  //cout << "nodes[149390805].first = " << nodes[X].first << endl;
  //cout << "nodes[149390806].first = " << nodes[X+1].first << endl;
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

  //for (intT i = 0; i < n; ++i) {
    //for (intT j = nodes[i].first; j < nodes[i+1].first; ++j) 
      //cout << i << " -> " << arcs[j].to << " (" << arcs[j].resCap << ")" << endl;
  //}

  init();
  t.stop(); cout << "t6 = " << t.total() << endl; t.clear(); t.start();
  timeInit.stop();
  cout << "init time: " << timeInit.total() << endl;
  beforeHook();
  run();
  afterHook();
  timer timeDeinit; timeDeinit.start();
  deinit();

  atomic<Cap> flow(0);
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
