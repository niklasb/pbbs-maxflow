#ifndef ARC_INCLUDED
#define ARC_INCLUDED

struct weightedArc {
  pair<uint,uint> arc;
  int weight;
weightedArc(uint u, uint v, int _weight) : arc(make_pair(u,v)), weight(_weight) {}
weightedArc() : arc(make_pair(0,0)),weight(0) {}
  inline uint u() { return arc.first; }
  inline uint v() { return arc.second; }
  inline void setU(uintT u) { arc.first = u;}
  inline void setV(uintT v) { arc.second = v;}
  void print() { 
    cout<<"("<<arc.first<<" "<<arc.second<<" "<<weight<<") "<<flush; }
} __attribute__ ((aligned (16)))
;

inline bool arcCASEdge(weightedArc* a, pair<uint,uint> b, pair<uint,uint> c) {
  return utils::CAS(&(a->arc),b,c);
}

inline bool arcCASWeight(weightedArc* a, int c) {
  return utils::xadd(&(a->weight),c);
  //return utils::CAS(&(a->weight),b,c);
}

#endif
