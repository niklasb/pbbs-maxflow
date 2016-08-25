#ifndef ARC_INCLUDED
#define ARC_INCLUDED

typedef pair<int,int> intPair;

struct weightedArc {
  intPair arc;
  int weight;
weightedArc(intT u, intT v, intT _weight) : arc(make_pair(u,v)), weight(_weight) {}
weightedArc() : arc(make_pair(0,0)),weight(-1) {}
  inline intT u() { return arc.first; }
  inline intT v() { return arc.second; }
  inline void setU(intT u) { arc.first = u;}
  inline void setV(intT v) { arc.second = v;}
  void print() { 
    cout<<"("<<arc.first<<" "<<arc.second<<" "<<weight<<") "<<flush; }
} __attribute__ ((aligned (16)))
;

inline bool arcCASEdge(weightedArc* a, intPair b, intPair c) {
  return utils::CAS(&(a->arc),b,c);
}

inline bool arcCASWeight(weightedArc* a, int c) {
  return utils::xaddi(&(a->weight),c);
  //return utils::CAS(&(a->weight),b,c);
}

#endif
