#ifndef KMEANSDEFS_H
#define KMEANSDEFS_H

#define N 2000000
#define D 60
#define K 1001
#define EPSILON 0

bool printscores = false;
bool printinfos = false;


typedef double flt;

flt data[N][D];
flt weights[N];
flt initial_seed[K][D];

flt xnormsqr[N];
flt clnormsqr[K];

flt sums[K][D];
int clusters[N];
int cnt[K];
flt moved[K];
flt dists[N];

flt sdist[K];
flt ubound[N];
flt lbound1[N];

// For hamerly_acc
int clind[N];
flt xnorm[N];
flt clnorm[N];

// For forgy
int randoms[K];

#endif
