#ifndef KMEANSBB_LLOYD_H
#define KMEANSBB_LLOYD_H

#include <iostream>
#include <fstream>
#include "parallel.h"
#include "gettime.h"
#include "kmeansbb.h"
#include "lloyd.h"
#include "kmeansIO.h"
#include "parseCommandLine.h"
#include "kmeansdefs.h"
#include "datautils.h"

/* A standard k-means algorithm that uses k-means|| for estimating the centers 
 * of each of the k clusters (seeding) and uses Lloyd's method for moving those
 * centers to a local optimum (iterations).
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int kmeansbbLloyd (F data[N][D], int num, int dim, int clusters, F error, 
		 int irounds, int kmbbrounds, F lfactor, F frac, int rseed,
		 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, INPD inprod,
		 F* bfscr, F* score, int* steps) {		
	// If succ != 0, the iterations failed.
	int succ = -1; 
	while (succ != 0) {
		// k-means|| for seeding
		kmeansbb (initial_seed, data, weights, num, dim, clusters, 
			distance, distancesqr, norm, normsqr, inprod, lfactor, rseed,
			kmbbrounds);

		// Lloyd's method for iterations
		succ = lloyd (data, weights, num, dim, clusters, initial_seed, 
			distance, distancesqr, norm, normsqr, inprod, error, score, bfscr, 
			steps, -1);
	}
}

#endif

#undef kmeans
#define kmeans(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps) (kmeansbbLloyd(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps))

