// config
const float globUpdtFreq = 2.0;
#define ALPHA 6
#define BETA 12

#include <iostream>
#include <queue>
#include <set>

struct Arc {
  intT to;
  intT rev;
  Cap resCap;
  Cap revResCap;
} *arcs;
intT arcId(Arc& a) { return min((intT)(&a-arcs), a.rev); }

struct Node {
  intT first;
  intT h;
  Cap e;
  intT last() const { return (this + 1)->first; }
  int inWorkingSet;
} *nodes;
template <typename N>
intT outDegree(N& n) { return n.last() - n.first; }
void resetArcPointers(Node& v) {}

#define for_arcs(for, n, a, body) \
  {for (intT _i = (n).first; _i < (n).last(); ++_i) { Arc& a = arcs[_i]; body }}

intT lowestChangedLabel;
ll nm;
intT n, m, source, sink;
ll globalRelabels;
ll otherWork, relabels, nonSatPushes, satPushes, edgeScans,
   discharges, globRelWork;
ll workSinceUpdate;
timer totalTime, sourceSaturationTime;
intT hiLabel;
queue<int> q;

void increaseArcFlow(Arc& a, Cap delta) {
  ass2(a.resCap-delta >= 0,(&a-arcs)<<" "<<a.to<<" "<<a.resCap<<" "<<delta);
  a.resCap -= delta;
  ass2(a.resCap >= 0,a.resCap<<" "<<delta);
  Arc& rev = arcs[a.rev];
  rev.revResCap = a.resCap;
  rev.resCap += delta;
  ass(rev.resCap>=0);
  a.revResCap = rev.resCap;
}

void init() {
  // n, m, source, sink and the graph structure stored in nodes, arcs must be
  // initialized already
  q.clear();
  for (intT i = 0; i < n; ++i) {
    Node& v = nodes[i];
    v.h = v.e = 0;
    resetArcPointers(v);
  }
  for (intT i = 0; i < m; ++i) {
    Arc& a = arcs[i];
    a.revResCap = arcs[a.rev].resCap;
  }
}

void deinit() {
  q.clear();
}

void checkDistanceLabels() {
  for (intT i = 0; i < n; ++i) {
    if (nodes[i].h == hiLabel) continue;
    for_arcs(for, nodes[i], a, {
      if (a.resCap > 0) {
        ass2(nodes[i].h <= nodes[a.to].h + 1,
               i << " " << a.to << " " << nodes[i].h << " " << nodes[a.to].h);
      }
    })
  }
}

void checkSanity() {
  cout << "Sanity check" << endl;
  ass(nodes[source].h == n);
  ass(nodes[sink].h == 0);
  checkDistanceLabels();
#if GAP_HEURISTIC
  checkLabelCounter();
#endif
  for (intT i = 0; i < n; ++i) {
    Node& v = nodes[i];
    for_arcs(for, v, a, {
      ass(a.resCap == arcs[a.rev].revResCap);
      ass(a.revResCap == arcs[a.rev].resCap);
      ass2(a.resCap >= 0, a.resCap);
      ass(arcs[a.rev].resCap >= 0);
    })
  }
}

void revBfs(intT exclude) {
  //cout << "BFS start" << endl;
  queue<int> q;
  q.push(sink);
  nodes[sink].h = 0;
  while (!q.empty()) {
    Node& v = nodes[q.front()];
    for_arcs(for,
  }
  timer t;
  t.clear();
  t.start();
  intT *degs = new intT[n], *offset = new intT[n];
  while (sz) {
    //cout << "level " << d << " " << sz << endl;
#if GAP_HEURISTIC
    labelCounter[d] = sz;
#endif
    parallel_for (intT i = 0; i < sz; ++i)
      degs[i] = outDegree(nodes[frontier[i]]);
    intT maxNewSz = sequence::plusScan(degs, offset, sz);
    globRelWork += maxNewSz;
    globRelIters++;
    intT *nxt = new intT[maxNewSz];
    forSwitch(400, i, 0, sz, {
      intT o = offset[i];
      Node& v = nodes[frontier[i]];
      forSwitch(1000, j, 0, degs[i], {
        Arc& ar = arcs[v.first + j];
        nxt[o + j] = -1;
        if (!ar.revResCap) continue;
        intT wi = ar.to;
        if (wi == exclude) continue;
        Node& w = nodes[wi];
        if (CAS(&w.h, hiLabel, v.h + 1)) {
          w.hnew = w.h;
          nxt[o + j] = wi;
        }
      })
    })
    sz = sequence::filter(nxt, frontier, maxNewSz, [](intT x) { return x >= 0; });
    delete[] nxt;
    d++;
  }
  delete[] degs;
  delete[] offset;
  t.stop();
  cout << "BFS end time " << t.total() << " diameter " << d << endl;
}

void globalRelabel() {
  cout << "global relabeling (low = " << lowestChangedLabel << ") ... " << flush;
  //cout << "hiLabel=" << hiLabel <<  endl;
  workSinceUpdate = 0;
  globalRelabelTime.start();
  globalRelabels++;

  parallel_for (intT i = 0; i < n; ++i) {
    Node& v = nodes[i];
    if (v.h > lowestChangedLabel || (lowestChangedLabel == 0 && i != sink))
      v.h = hiLabel;
#if GAP_HEURISTIC
    if (i > lowestChangedLabel)
      labelCounter[i] = 0;
#endif
  }

  intT *frontier = new intT[n];
  intT sz = 0;
  if (lowestChangedLabel == 0)
    frontier[sz++] = sink;
  else {
    intT *tmp = new intT[n];
    parallel_for (intT vi = 0; vi < n; ++vi) {
      Node& v = nodes[vi];
      if (v.h == lowestChangedLabel)
        tmp[vi] = vi;
      else
        tmp[vi] = -1;
    }
    sz = sequence::filter(tmp, frontier, n, [](intT x) { return x >= 0; });
    delete[] tmp;
  }
  revBfs(frontier, sz, lowestChangedLabel, source);
  delete[] frontier;
  globalRelabelTime.stop();
#if INCREMENTAL_GLREL
  lowestChangedLabel = numeric_limits<intT>::max();
#endif
  cout << "done" << endl;
}

void printStats() {
  cout << setprecision(4) << fixed;
#define FIELD(name, value) cout << "  " << left << setw(30) << (name) \
         << right << setw(20) << fixed << setprecision(2) << (value) << endl
  cout << "after " << pass << " iterations:" << endl;
  FIELD("nodes", n);
  FIELD("edges", m/2);
  FIELD("sink excess", nodes[sink].e);
  cout << endl;
  FIELD("active nodes", wSetSize);
  FIELD("average active", pass > 0 ? to_string(sumActive / pass) : "n/a");
  cout << endl;
  FIELD("global relabels", globalRelabels);
  FIELD(GAP_HEURISTIC ? "gaps (enabled)" : "gaps (disabled)", gaps);
#if !OPENMP && GAP_HEURISTIC
  if (stats) {
    FIELD("* gap nodes", gapNodes.get_value());
    FIELD("* gap excess", gapNodeExcess.get_value());
  }
#endif
  cout << endl;
  FIELD("total time", totalTime.total());
  FIELD("* source time", sourceSaturationTime.total());
  FIELD("* glrel time", globalRelabelTime.total());
  FIELD("* phase1 time", phaseTime[0].total());
  FIELD("* phase2 time", phaseTime[1].total());
  FIELD("* phase3 time", phaseTime[2].total());
  FIELD("* phase4 time", phaseTime[3].total());
  FIELD("* phase5 time", phaseTime[4].total());
  FIELD("* phase6 time", phaseTime[5].total());
  FIELD("* phase7 time", phaseTime[6].total());
  FIELD("* phase8 time", phaseTime[7].total());
  FIELD("* groupBy time 1", tGroupBy[0].total());
  FIELD("* groupBy time 2", tGroupBy[1].total());
  FIELD("* groupBy time 3", tGroupBy[2].total());
  FIELD("* gap heuristic time", gapHeuristicTime.total());
  FIELD("groupBy avg", (double)sumGroupBy / cntGroupBy);
  FIELD("groupBy min", groupByMin);
  FIELD("glrel work / iters", (double)globRelWork / globRelIters);
#if !OPENMP
  if (stats) {
    FIELD("edge scans", edgeScans.get_value());
    FIELD("edge scans / discharge", 1.0 * edgeScans.get_value() / discharges);
    FIELD("discharges", discharges);
    //FIELD("* phase3 time", phase3Time.total());
    cout << endl;
    FIELD("pushes + relabels", satPushes.get_value() + nonSatPushes.get_value() + relabels.get_value());
    FIELD("* relabels", relabels.get_value());
    FIELD("* relabels / discharge", (double)relabels.get_value() / discharges);
    FIELD("* pushes", satPushes.get_value() + nonSatPushes.get_value());
    FIELD("  -> saturating", satPushes.get_value());
    FIELD("  -> non-saturating", nonSatPushes.get_value());
    cout << endl;
    FIELD("total work", globRelWork + otherWork.get_value());
    FIELD("* work glrel", globRelWork);
    FIELD("* other work", otherWork.get_value());
  }
#endif
  cout << endl;
}

bool needGlobalUpdate() {
  return workSinceUpdate * globUpdtFreq > nm;
}

ll processNode(intT i, intT vi) {
  ass(vi != source && vi != sink);

  //cout << "Processing " << vi << endl;
  bool dbg=0;
  //if (dbg) cout << "(1)processing " << vi << " del=" << del << endl;
  ll addRelabels = 0, addSatPushes = 0, addNonSatPushes = 0,
     addOtherWork = 0, addWorkSinceUpdate = 0,
     addEdgeScans = 0;
  Node& vorig = nodes[vi];
  ass(vorig.e > 0);
  if (vorig.h == hiLabel) {
    vorig.pushesEnd = vorig.pushes;
    vorig.hnew = vorig.h;
    vorig.inWorkingSet = 0;
#if GAP_HEURISTIC
    // TODO fix the bug in groupBy when this happens
    vorig.relabelsEnd = vorig.relabels;
#endif
    return 0;
  }
  //if (dbg) cout << "processing " << vi << " (excess " << vorig.e << ")"  << endl;
  NodeView v = vorig.view();
  //if (dbg) cout << "  v.e = " << v.e << endl;
  ass(v.e > 0);
  int relabel = 0;
#if INCREMENTAL_GLREL
  intT lowestChangedL = numeric_limits<intT>::max();
#endif
  while (v.e > 0) {
    intT maxLabel = hiLabel;
    bool skipped = 0;
    for_arcs(for, v, cur, {
      if (stats) addOtherWork++;
      if (!v.e) break;
      addEdgeScans++;
      intT wi = cur.to;
      Node& w = nodes[wi];
      //if (dbg) cout << "inspecting " << vi <<"->" << wi << " (c=" << c << ", dc=" << dc << ")" << endl;
      //if (w.e && w.h == vorig.h + 1) {
      bool admissible = v.h == w.h + 1;
      if (w.e) {
        // TODO what if vorig.h == w.h? In that case nobody wins
        if (w.h < vorig.h - 1 || w.h == vorig.h + 1) {
          // in this scenario, w could be relabeled above v and manipulate the
          // edge capacity, leading to non-determinism depending on whether
          // v gets relabeled first or w pushes first to v
          maxLabel = min(maxLabel, vorig.h + 2);
        }
        bool win = vorig.h == w.h + 1 || vorig.h < w.h - 1;
        if (!win && admissible) {
          skipped = 1;
          continue;
        }
      }

      if (!admissible) {
        // inadmissible
        continue;
      }

      Cap c = cur.resCap;
      if (!c || v.h != w.h + 1) {
        continue;
      }
      //if (w.h == vorig.h + 1) {
        //// w owns this edge for now (until it is relabelled, then it will never
        //// push along the edge again because it is higher than vorig.h + 1)
        //skipped = 1;
        //// try again later
        //continue;
      //}
      Cap delta = min(c, v.e);
      if (wi != sink) {
#if P2L
        Cap p2lCap;
        if (mode == MODE_RACE)
          p2lCap = max((Cap)0, w.sumOutCap - enew[wi] - w.e - cur.revResCap);
        else
          p2lCap = max((Cap)0, w.sumOutCap - w.e - cur.revResCap);
        delta = min(delta, p2lCap);
#endif
#if EXCESS_SCALING
        Cap dc;
        if (mode == MODE_RACE)
          dc = max((Cap)0, del - enew[wi] - w.e);
        else
          dc = max((Cap)0, del - w.e);
        if (dc < delta) {
          delta = dc;
          skipped = 1;
        }
#endif
      }
      if (!delta) continue;
      ass(delta > 0);
      if (dbg) ATOMIC(cout << "push " << vi << " (" << v.e << ") -> " << wi << ": " << delta << endl;)
      if (stats) {
        if (delta == c) ++addSatPushes;
        else            ++addNonSatPushes;
      }
      v.e -= delta;
      ass(v.e >= 0);
      increaseArcFlow(cur, delta);
#if INCREMENTAL_GLREL
      lowestChangedL = min(lowestChangedL, w.h);
#endif
      if (mode == MODE_RACE) {
        enew[wi] += delta;
#if P2L
        vorig.sumOutCap -= delta;
        w.sumOutCap += delta;
#endif
        if ((!w.e || wi == sink) && CAS(&w.inWorkingSet,0,1))
          *v.pushes++ = make_pair(wi, 0);
      } else {
        *v.pushes++ = make_pair(wi, delta);
      }
    })
    if (!v.e) goto done;
    if (skipped) goto done;
    // relabel
    intT newh = maxLabel;
    addWorkSinceUpdate += BETA + outDegree(v);
    if (stats) addOtherWork += outDegree(v);
    for_arcs(for, v, a, {
      Node& w = nodes[a.to];
#if P2L
      if (min(a.resCap, w.sumOutCap - enew[a.to]- w.e - a.revResCap) > 0 && w.h >= v.h)
#else
      if (a.resCap && w.h >= v.h)
#endif
        newh = min(newh, w.h + 1);
    });
    if (stats) addRelabels++;
#if GAP_HEURISTIC
    if (newh < hiLabel) {
      *v.relabels++ = v.h;
      *v.relabels++ = hiLabel + newh;
    }
#endif
    if (dbg) {
      ATOMIC(cout << "relabel " << vi << " from " << v.h << " to "
              << newh << " (excess left=" << v.e << ", relabel="
              << ++relabel << ", skipped=" << skipped << " maxLabel=" << maxLabel << ")" << endl;)
    }
    v.h = newh;
    resetArcPointers(v);
    if (v.h == maxLabel) break;
  }

done:
#if INCREMENTAL_GLREL
  writeMin(&lowestChangedLabel, lowestChangedL);
#endif
  if (mode == MODE_RACE) {
    enew[vi] += v.e - vorig.e;
    *v.pushes++ = make_pair(vi, 0);
  } else {
    *v.pushes++ = make_pair(vi, v.e - vorig.e);
  }
  nodes[vi].apply(v);
#if !OPENMP
  if (stats) {
    relabels += addRelabels;
    satPushes += addSatPushes;
    nonSatPushes += addNonSatPushes;
    otherWork += addOtherWork;
    edgeScans += addEdgeScans;
  }
#endif
  return addWorkSinceUpdate;
}

void run() {
#if !OPENMP
  otherWork.set_value(0);
  satPushes.set_value(0);
  nonSatPushes.set_value(0);
  relabels.set_value(0);
  gapNodeExcess.set_value(0);
  gapNodes.set_value(0);
  edgeScans.set_value(0);
#endif
  discharges = 0;

  globalRelabels = 0;
  gaps = 0;
  globRelWork = 0;
  globRelIters = 0;

  totalTime.clear();
  globalRelabelTime.clear();
  sourceSaturationTime.clear();
  gapHeuristicTime.clear();
  for (int i = 0; i < 8; ++i) phaseTime[i].clear();

  sumActive = 0;
  hiLabel = n;
  nm = ALPHA * (ll)n + (ll)m/2;
  pass = 0;

#ifdef INITIAL_RELABEL
  workSinceUpdate = numeric_limits<ll>::max();
#else
  workSinceUpdate = 0;
#endif

  totalTime.start();
  sourceSaturationTime.start();

  int oldNumThreads = getWorkers();
  int numThreads = getWorkers();
  bool sequential = 0;
  Node& S = nodes[source];
  S.h = n;
  Cap maxExcess = 0;
  // TODO allow multi-edges
  parallel_for (intT i = 0; i < outDegree(S); ++i) {
    Arc& a = arcs[S.first + i];
    ass(a.to != source);
    wSetTmp[i] = a.to;
    Cap delta = a.resCap;
    if (!delta) continue;
    ass(delta > 0);
    increaseArcFlow(a, delta);
    nodes[a.to].e += delta;
#if EXCESS_SCALING
    writeMax(&maxExcess, nodes[a.to].e);
#endif
  }
  wSetSize = sequence::filter(wSetTmp, wSet, outDegree(S),
                              [](intT vi) { return vi != sink && nodes[vi].e > 0; });
  wSetSize = unique(wSet, wSet + wSetSize) - wSet;
  lowestChangedLabel = 0;
#if EXCESS_SCALING
  cout << "initial max excess: " << maxExcess << endl;
  del = 1;
  while (del < maxExcess)
    del <<= 1;
#endif
#if !OPENMP
  satPushes += outDegree(S);
#endif
  sourceSaturationTime.stop();

#if P2L
  parallel_for (intT vi = 0; vi < n; ++vi) {
    Node& v = nodes[vi];
    for_arcs(for, v, cur, {
        v.sumOutCap += cur.resCap;
    });
  }
#endif
  mode = MODE_RACE;
  while (wSetSize) {
    //cout << "iteration " << pass << " active " << wSetSize << endl;
    //cout << "  active="; for (intT i= 0; i < wSetSize; ++i) cout << setw(3) << wSet[i] << " ";cout << endl;
    //cout << "  e="; for (intT vi = 0; vi < n; ++vi) cout << setw(3) << nodes[vi].e << " ";cout << endl;
    //cout << "  h="; for (intT vi = 0; vi < n; ++vi) cout << setw(3) << nodes[vi].h << " ";cout << endl;
    if (statsInterval > 0 && pass % statsInterval == 0) printStats();
    sumActive += wSetSize;

    if (wSetSize < 50 && !sequential) {
      sequential = 1;
      cout<< "Falling back to sequential after " << totalTime.total() << endl;
      //mode = MODE_RACE;
      setWorkers(1);
      //cout << "Falling back to augmenting paths" << endl;
      //timer dinicTime; dinicTime.start();
      //dinic::run();
      //dinicTime.stop();
      //cout << "Dinic: " << dinicTime.total() << endl;
      //break;
    } else if (wSetSize > 50 && sequential) {
      sequential = 0;
      cout<< "Back to parallel after " << totalTime.total() << endl;
      setWorkers(oldNumThreads);
    }

    //checkSanity();
    if (needGlobalUpdate())
      globalRelabel();
    //cout << "active = "; s.process([](intT i, intT vi) { ATOMIC(cout << vi << " ";); }); cout << endl;

    phaseTime[7].start();
    phaseTime[0].start();
    //cout << "ITER" << endl;
    discharges += wSetSize;
    parallel_for (intT i = 0; i < wSetSize; ++i) {
      intT vi = wSet[i];
      workSinceUpdateA[i] = processNode(i, vi);
      bufSize[i] = nodes[vi].pushesEnd - nodes[vi].pushes;
    }
    phaseTime[0].stop();
    phaseTime[7].stop();

#if GAP_HEURISTIC
    gapHeuristicTime.start();
    intT *relSizes = new intT[wSetSize], *relOffset = new intT[wSetSize];
    parallel_for (intT i = 0; i < wSetSize; ++i) {
      Node& v = nodes[wSet[i]];
      relSizes[i] = v.relabelsEnd - v.relabels;
    }
    intT k = sequence::plusScan(relSizes, relOffset, wSetSize);
    intT *allRelabels = new intT[k], *relFirst = new intT[k + 1];
    allRelabels[k] = hiLabel;
    parallel_for (intT i = 0; i < wSetSize; ++i) {
        Node& v = nodes[wSet[i]];
        copy(v.relabels, v.relabelsEnd, allRelabels + relOffset[i]);
    }
    k = groupBy(allRelabels, [](intT x) { return x; }, relFirst, k, hiLabel);
    intT mid = 0;
    parallel_for (intT i = 0; i < k; ++i) {
      intT label1 = allRelabels[relFirst[i]];
      intT label2 = allRelabels[relFirst[i+1]];
      if (label1 < hiLabel && label2 >= hiLabel) mid = i+1;
    }
    ass(mid >= 0);
    intT gap = hiLabel;
    parallel_for (intT i = 0; i < mid; ++i) {
      intT label = allRelabels[relFirst[i]];
      intT count = relFirst[i+1] - relFirst[i];
      labelCounter[label] -= count;
      if (labelCounter[label] == 0)
        writeMin(&gap, label);
    }
    parallel_for (intT i = mid; i < k; ++i) {
      intT label = allRelabels[relFirst[i]];
      intT count = relFirst[i+1] - relFirst[i];
      labelCounter[label-hiLabel] += count;
    }
    delete[] relSizes;
    delete[] relOffset;
    delete[] allRelabels;
    delete[] relFirst;

    if (gap < hiLabel) {
      gaps++;
      cout << "GAP at " << gap << "!" << endl;
      parallel_for(intT vi = 0; vi < n; ++vi) {
        Node& v = nodes[vi];
        //ass(v.h != gap); // needn't be the case
        if (v.h > gap) {
          v.h = hiLabel;
#if !OPENMP // uses Cilk reducer
          if (stats) {
            gapNodes++;
            gapNodeExcess += v.e + v.enew;
          }
#endif
        }
      }
    }
    gapHeuristicTime.stop();
#endif

    phaseTime[7].start();
    phaseTime[1].start();
    workSinceUpdate += sequence::plusReduce(workSinceUpdateA, wSetSize);
    //workSinceUpdate += sequence::mapReduce<ll>(workSinceUpdateA, wSetSize, utils::addF<ll>(),
                                            //[](const PaddedLL& x) { return x.x; });
    offset[wSetSize] = sequence::plusScan(bufSize, offset, wSetSize);
    phaseTime[1].stop();
    if (mode == MODE_SYNC) {
      ass(!EXCESS_SCALING && !P2L); // TODO implement
      phaseTime[2].start();
      if (wSetSize > 300) {
        parallel_for (intT i = 0; i < wSetSize; ++i) {
          Node& v = nodes[wSet[i]];
          v.h = v.hnew;
          copy(v.pushes, v.pushesEnd, groupedPushes + offset[i]);
        }
      } else {
        for (intT i = 0; i < wSetSize; ++i) {
          Node& v = nodes[wSet[i]];
          v.h = v.hnew;
          copy(v.pushes, v.pushesEnd, groupedPushes + offset[i]);
        }
      }
      phaseTime[2].stop();
      phaseTime[3].start();
      wSetSize = groupBy(groupedPushes, [](const pair<ll,Cap>& x) { return x.first; },
                        first, offset[wSetSize], n);
      phaseTime[3].stop();
      phaseTime[4].start();
      if (wSetSize > 300) {
        parallel_for(intT i = 0; i < wSetSize; i++) {
          intT vi = groupedPushes[first[i]].first;
          Node& v = nodes[vi];
          for (intT j = first[i]; j < first[i+1]; ++j)
            v.e += groupedPushes[j].second;
          wSetTmp[i] = vi;
        }
      } else {
        for (intT i = 0; i < wSetSize; i++) {
          intT vi = groupedPushes[first[i]].first;
          Node& v = nodes[vi];
          intT sz = first[i+1]-first[i];
          if (sz > 300) {
            v.e += sequence::mapReduce<Cap>(groupedPushes + first[i], sz, utils::addF<Cap>(),
                                            [](const pair<ll, Cap>& x) { return x.second; });
          } else {
            for (intT j = first[i]; j < first[i+1]; ++j)
              v.e += groupedPushes[j].second;
          }
          wSetTmp[i] = vi;
        }
      }
      phaseTime[4].stop();
    } else { // MODE_RACE
      phaseTime[2].start();
      if (wSetSize > 300) {
        parallel_for (intT i = 0; i < wSetSize; ++i) {
          intT vi = wSet[i];
          Node& v = nodes[vi];
          v.h = v.hnew;
          intT sz = v.pushesEnd - v.pushes;
          intT o = offset[i];
          for (intT j = 0; j < sz; ++j)
            wSetTmp[o++] = v.pushes[j].first;
        }
      } else {
        for (intT i = 0; i < wSetSize; ++i) {
          intT vi = wSet[i];
          Node& v = nodes[vi];
          v.h = v.hnew;
          intT sz = v.pushesEnd - v.pushes;
          intT o = offset[i];
          for (intT j = 0; j < sz; ++j)
            wSetTmp[o++] = v.pushes[j].first;
        }
      }
      phaseTime[2].stop();
      phaseTime[3].start();
      wSetSize = offset[wSetSize];
      Cap maxExcess = 0;
      parallel_for (intT i = 0; i < wSetSize; ++i) {
        intT vi = wSetTmp[i];
        Node& v = nodes[vi];
        v.e += enew[vi];
        enew[vi] = 0;
        v.inWorkingSet = 0;
#if EXCESS_SCALING
        if (vi != sink && v.h < hiLabel)
          writeMax(&maxExcess, v.e);
#endif
      }
#if EXCESS_SCALING
      if (maxExcess <= del/2) {
        del /= 2;
        cout << "new del = " << del << " (time " << totalTime.total() << ")" << endl;
      }
#endif
      phaseTime[3].stop();
    }
    phaseTime[5].start();
    if (wSetSize > 1000) {
      wSetSize = sequence::filter(wSetTmp, wSet, wSetSize,
          [](intT vi) {
            return nodes[vi].e > 0 &&
                    nodes[vi].h < hiLabel &&
                    vi != sink;
          });
    } else {
      int j = 0;
      for (intT i = 0; i < wSetSize; ++i) {
        intT vi = wSetTmp[i];
        if (nodes[vi].e > 0 && nodes[vi].h < hiLabel && vi != sink)
          wSet[j++] = vi;
      }
      wSetSize = j;
    }
    phaseTime[5].stop();
    phaseTime[7].stop();

    //t.stop(); cout << " t3 = " << t.total(); t.clear(); t.start();
    //cout << endl;
    ++pass;
  }
  totalTime.stop();
  printStats();
  cout << "Resetting number of threads to " << oldNumThreads << endl;
  setWorkers(oldNumThreads);
}

intT maxFlow(FlowGraph<intT>& g) {
  //for (intT i = 0; i < g.g.n; ++i) {
    //FlowVertex& v = g.g.V[i];
    //for (intT j = 0; j < v.degree; ++j) {
      //intT to = v.Neighbors[j];
      //intT c = v.nghWeights[j];
      //cout << i << " -> " << to << " (" << c << ")" << endl;
    //}
  //}
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
  Cap *cap = new Cap[m / 2];
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

  //atomic<Cap> flow(0);
  //parallel_for (intT i = 0; i < n; ++i) {
    //FlowVertex& v = g.g.V[i];
    //parallel_for (intT j = 0; j < v.degree; ++j) {
      //v.nghWeights[j] = arcs[v.nghWeights[j]].resCap;
      //if (v.Neighbors[j] == sink) flow += v.nghWeights[j];
    //}
  //}
  //ass(flow == nodes[sink].e);
  Cap flow = nodes[sink].e;
  cout << "flow=" << flow << endl;
  delete[] nodes;
  delete[] arcs;
  timeDeinit.stop();
  cout << "deinit time: " << timeDeinit.total() << endl;
  return flow;
}
