#include "geom.h"
#include "seq.h"

class particle {
public:
  point3d pt;
  double mass;
  vect3d force;
  particle(point3d p, double m) : pt(p),mass(m) {}
};

// gravitational constant
#define gGrav 1.0  

void step(seq<particle*> particles);
void stepReference(seq<particle*> particles);
