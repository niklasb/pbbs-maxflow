#ifndef ARC_INCLUDED
#define ARC_INCLUDED

typedef pair<intT,intT> intPair;

struct weightedArc {
  intPair arc;
  intT weight;
  weightedArc(intT u, intT v, intT _weight) : arc(make_pair(u,v)), weight(_weight) {}
weightedArc() : arc(make_pair(0,0)),weight(-1) {}
  intT u() { return arc.first; }
  intT v() { return arc.second; }
  void setU(intT u) { arc.first = u;}
  void setV(intT v) { arc.second = v;}
};


struct weightedArcArray {
  weightedArc* E;
  intT n;
  intT m;
  void del() {free(E);}
  weightedArcArray(weightedArc *EE, intT _n, intT _m) :
  E(EE), n(_n), m(_m) {}
  weightedArcArray() {}
};


#endif
