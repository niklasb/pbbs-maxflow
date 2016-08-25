#include <iostream>
#include <algorithm>
#include "cilk.h"
#include "geom.h"
#include "sequence.h"
#include "gettime.h"
using namespace std;
using namespace sequence;

struct belowLine {
  point2d l,r;
  belowLine(point2d _l, point2d _r) : l(_l), r(_r) {}
  bool operator() (point2d p) {return triArea(l,r,p) > 0;}
};

struct triangArea {
  point2d l,r,*P;
  triangArea(point2d* _P, point2d _l, point2d _r) : P(_P), l(_l), r(_r) {}
  double operator() (int i) {return triArea(l,r,P[i]);}
};

struct getX {
  point2d *P;
  getX(point2d* _P) : P(_P) {}
  double operator() (int i) {return P[i].x;}
};

int quickHull(point2d* Pin, point2d* Tmp, int n, point2d l, point2d r) {
  if (n == 0) return 0;
  else if (n == 1) {
    return 1;
  } else {
    int idx = maxIndex<double>(0,n,greater<double>(),triangArea(Pin,l,r));
    point2d maxP = Pin[idx];

    int n1,n2,m1,m2;
    if (n > 5000) { // cuttoff chosen empiriclally
      n1 = filter(Pin, Tmp, n, belowLine(l,maxP));
      n2 = filter(Pin, Tmp+n1, n, belowLine(maxP,r));

      m1 = cilk_spawn quickHull(Tmp,Pin,n1,l,maxP);
      m2 = quickHull(Tmp+n1,Pin+n1,n2,maxP,r);
      cilk_sync;

      cilk_for (int i=0; i < m1; i++) Pin[i] = Tmp[i];
      Pin[m1] = maxP;
      cilk_for (int i=0; i < m2; i++) Pin[i+m1+1] = Tmp[i+n1];
      return m1+1+m2;

    } else {
      int ll = 0, lm = 0;
      int rm = n-1, rr = n-1;

      while (1) {
        while ((lm <= n) && !(triArea(maxP,r,Pin[lm]) > 0)) {
	  if (triArea(l,maxP,Pin[lm]) > 0) Pin[ll++] = Pin[lm];
	  lm++;
	}
        while ((rm >= 0) && !(triArea(l,maxP,Pin[rm]) > 0)) {
	  if (triArea(maxP,r,Pin[rm]) > 0) Pin[rr--] = Pin[rm];
	  rm--;
	}
	if (lm >= rm) break; 
        point2d tmp = Pin[lm++];
	Pin[ll++] = Pin[rm--];
	Pin[rr--] = tmp;
      }
      int n1 = ll;
      int n2 = n-rr-1;

      m1 = quickHull(Pin,NULL,n1,l,maxP);
      m2 = quickHull(Pin+n-n2,NULL,n2,maxP,r);

      for (int i=0; i < m2; i++) Pin[i+m1+1] = Pin[i+n-n2];
      Pin[m1] = maxP;
      return m1+1+m2;
    }
  }
}

int Hull(point2d* Pin, point2d* Pout, int n) {
  int minIdx = maxIndex<double>(0,n,less<double>(),getX(Pin));
  int maxIdx = maxIndex<double>(0,n,greater<double>(),getX(Pin));
  point2d l = Pin[minIdx];
  point2d r = Pin[maxIdx];

  bool* fTop = newA(bool,n);
  bool* fBot = newA(bool,n);
  cilk_for(int i=0; i < n; i++) {
    double a = triArea(l,r,Pin[i]);
    fTop[i] = a > 0;
    fBot[i] = a < 0;
  }

  int n1 = pack(Pin,Pout,fTop,n);
  int n2 = pack(Pin,Pout+n1,fBot,n);
  free(fTop); free(fBot);

  int m1 = quickHull(Pout,Pin,n1,l,r);
  int m2 = quickHull(Pout+n1,Pin+n1,n2,r,l);

  cilk_for (int i=0; i < m1; i++) Pin[i+1] = Pout[i];
  cilk_for (int i=0; i < m2; i++) Pin[i+m1+2] = Pout[i+n1];
  Pin[0] = l;
  Pin[m1+1] = r;
  return m1+2+m2;
}

