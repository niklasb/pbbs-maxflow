//cost for match
static int MATCH = 2;

//cost for mismatch
static int MISMATCH = -1;

//cost for space
static int SPACE = -1;

//objective function
inline intT f(char c1, char c2) {
  if(c1 == c2) return MATCH;
  else return MISMATCH;
}

//max of 4 arguments
inline intT max4(intT a, intT b, intT c, intT d) {
  return max(a,max(b,max(c,d)));
}

intT localAlignment(unsigned char* s1, long n, unsigned char* s2, long m);
