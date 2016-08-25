#ifndef ARC_INCLUDED
#define ARC_INCLUDED

struct weightedArc {
  pair<uint,uint> arc;
  long weight;
weightedArc(uint u, uint v, long _weight) : arc(make_pair(u,v)), weight(_weight) {}
weightedArc() : arc(make_pair(0,0)),weight(-1) {}
  inline uint u() { return arc.first; }
  inline uint v() { return arc.second; }
  inline void setU(uint u) { arc.first = u;}
  inline void setV(uint v) { arc.second = v;}
  void print() { 
    cout<<"("<<arc.first<<" "<<arc.second<<" "<<weight<<") "<<flush; }
} __attribute__ ((aligned (16)))
;

inline bool arcCAS(weightedArc* a, weightedArc b, weightedArc c) {
  weightedArc* bptr = &b; weightedArc* cptr = &c;
  __int128* b128 = (__int128*)bptr; 
  __int128* c128 = (__int128*)cptr;
  return __sync_bool_compare_and_swap((__int128*)a,*b128,*c128);
}

#endif
