#ifndef SAMPLE_LLOYD_H
#define SAMPLE_LLOYD_H

#include <iostream>
#include <fstream>
#include "parallel.h"
#include "gettime.h"
#include "sample.h"
#include "lloyd.h"
#include "kmeansIO.h"
#include "parseCommandLine.h"
#include "kmeansdefs.h"
#include "datautils.h"

/* A k-means algorithm that uses sample to estimate the center of each of the
 * k clusters (seeding) and uses Lloyd's method to move those clusters toward
 * a local optimum (iterations).
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int sampleLloyd (F data[N][D], int num, int dim, int clusters, F error, 
		 int irounds, int kmbbrounds, F lfactor, F frac, int rseed,
		 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, INPD inprod,
		 F* bfscr, F* score, int* steps) {		
	// If succ != 0, the iterations failed.
	int succ = -1; 
	while (succ != 0) {
		// Sample for seeding
		sample (initial_seed, data, weights, num, dim, clusters, 
			distance, distancesqr, norm, normsqr, inprod, rseed, frac);

		// Lloyd's method for iterations
		succ = lloyd (data, weights, num, dim, clusters, initial_seed, 
			distance, distancesqr, norm, normsqr, inprod, error, score, bfscr, 
			steps, -1);
	}
}

#endif

#undef kmeans
#define kmeans(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps) (sampleLloyd(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps))


