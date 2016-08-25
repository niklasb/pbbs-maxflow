#define NODES_PER_ITER 256

intT excludeNode;

#define SPAWN(X) do{if (serial){X;}else{cilk_spawn X;}}while(0)
#define SYNC if (!serial) cilk_sync

template <typename F>
static inline void
procNode(const intT n[], intT fillSize, F f) {
  for (intT j = 0; j < fillSize; ++j)
    f(n[j]);
}

template <typename F>
inline void
walkPennant(Pennant<intT>* p, F f, bool serial) {
  if (p->getLeft())
    SPAWN(walkPennant(p->getLeft(), f, serial));
  if (p->getRight())
    SPAWN(walkPennant(p->getRight(), f, serial));

  const intT *n = p->getElements();

  // this works as long as NODES_PER_ITER divides BLK_SIZE
  cilk_for_1 (intT i = 0; i < BLK_SIZE; i+=NODES_PER_ITER)
    procNode(n+i, NODES_PER_ITER, f);
  delete p;
}

template <typename F>
inline void
bagApply(Bag<intT>& b, F f, bool serial) {
  if (b.getFill()) {
    // Split the bag and recurse
    Pennant<intT> *p;
    b.split(&p); // destructive split
    SPAWN(bagApply(b, f, serial));
    walkPennant(p, f, serial);
    SYNC;
  } else {
    intT fillSize = b.getFillingSize();
    const intT *n = b.getFilling();

    intT extraFill = fillSize % NODES_PER_ITER;
    SPAWN(procNode(n + fillSize - extraFill, extraFill, f));
    cilk_for_1 (intT i = 0; i < fillSize - extraFill; i += NODES_PER_ITER)
      procNode(n+i, NODES_PER_ITER, f);
    SYNC;
  }
}
