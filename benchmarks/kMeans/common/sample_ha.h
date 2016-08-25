#ifndef SAMPLE_HAMERLY_ACC_H
#define SAMPLE_HAMERLY_ACC_H

#include <iostream>
#include <fstream>
#include "parallel.h"
#include "gettime.h"
#include "sample.h"
#include "hamerly_acc.h"
#include "kmeansIO.h"
#include "parseCommandLine.h"
#include "kmeansdefs.h"
#include "datautils.h"

/* A k-means algorithm that uses sample to estimate the centers of each of
 * the k clusters (seeding) and uses the accelerated Hamerly's method to
 * move those clusters toward a local optimum (iterations).
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int sampleHamerlyAcc (F data[N][D], int num, int dim, int clusters, F error, 
		 int irounds, int kmbbrounds, F lfactor, F frac, int rseed,
		 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, INPD inprod,
		 F* bfscr, F* score, int* steps) {		
	// If succ != 0, the iterations failed.
	int succ = -1; 
	while (succ != 0) {
		// Sample for seeding
		sample (initial_seed, data, weights, num, dim, clusters, 
			distance, distancesqr, norm, normsqr, inprod, rseed, frac);

		// Accelerated Hamerly's method for iterations
		succ = hamerly_acc (data, weights, num, dim, clusters, initial_seed, 
			distance, distancesqr, norm, normsqr, inprod, error, score, bfscr, 
			steps, -1);
	}
}

#endif

#undef kmeans
#define kmeans(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps) (sampleHamerlyAcc(data, num, dim, clusters, error, irounds, kmbbrounds, lfactor, frac, rseed, distance, distancesqr, norm, normsqr, inprod, bfscr, score, steps))


