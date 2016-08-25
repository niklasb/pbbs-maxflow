#ifndef SAMPLE_H
#define SAMPLE_H

#include "parallel.h"
#include "sequence.h"
#include "kmeansdefs.h"
#include "kmeansutils.h"
#include "lloyd.h"
#include "kmeanspp.h"

using namespace std;
using namespace sequence;

/* A seeding algorithm that simply samples some fraction of the data points,
 * runs k-means of that sample, and uses the centers determined by that
 * execution of k-means as the seed for iterative methods base on Lloyd's 
 * method.
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
void sample (F seed[][D], F data[][D], F weights[], int n, int d, int k, 
						 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr,
						 INPD inprod, int rseed, double p) {
	for (int i = 0; i < n; i++) include[i] = -1;

	// Sample each point independently with equal probability less than 1 
	parallel_for (int i = 0; i < n; i++) {
		double pr = ((double) utils::hashInt (i + rseed) / INT_MAX);
		if (pr <= p) include[i] = i;
	}

	// Determine the number of points sampled
	int newn = filter (include, centers, n, notNeg<int>());

	parallel_for (int i1 = 0; i1 < newn; i1++) {
		for (int j = 0; j < d; j++) datap[i1][j] = data[centers[i1]][j];
	}

	// Run k-means on that sample
	kmeanspp (seed, datap, weights, newn, d, k, distance, distancesqr, norm, 
						normsqr, inprod, rseed);
	lloyd <F, DST, DSTSQ, NRM, NRMSQ, INPD>
		(datap, weights, newn, d, k, seed, distance, distancesqr, norm, normsqr, inprod,
		 0.0, NULL, NULL, NULL, -1);
}

#endif
