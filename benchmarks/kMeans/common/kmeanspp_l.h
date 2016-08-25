#ifndef KMEANSPP_LLOYD_H
#define KMEANSPP_LLOYD_H

#include <iostream>
#include <fstream>
#include "parallel.h"
#include "gettime.h"
#include "kmeanspp.h"
#include "lloyd.h"
#include "kmeansIO.h"
#include "parseCommandLine.h"
#include "kmeansdefs.h"
#include "datautils.h"

/* A standard k-means algorithm that uses k-means++ to estimate the centers
 * of each of the k clusters (seeding) and uses Lloyd's method to move those
 * centers toward a local optimum (iterations).
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int kmeansppLloyd (F data[N][D], int num, int dim, int clusters, F error, 
		 int irounds, int kmbbrounds, F lfactor, F frac, int rseed,
		 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, INPD inprod,
		 F* bfscr, F* score, int* steps) {		
	// If succ != 0, the iterations failed.
	int succ = -1; 
	while (succ != 0) {
		// K-means++ for seeding
		kmeanspp (initial_seed, data, weights, num, dim, clusters, 
			distance, distancesqr, norm, normsqr, inprod, rseed);

		// Lloyd's method for iterations
		succ = lloyd (data, weights, num, dim, clusters, initial_seed, 
			distance, distancesqr, norm, normsqr, inprod, error, score, bfscr, 
			steps, -1);
	}
}

#endif

#undef kmeans
#define kmeans(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps) (kmeansppLloyd(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps))


