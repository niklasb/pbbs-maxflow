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
