#ifndef KMEANSSS_H
#define KMEANSSS_H

#include "parallel.h"
#include "sequence.h"
#include "lloyd.h"
#include "kmeanspp.h"
#include "kmeansdefs.h"
#include "kmeansutils.h"
using namespace std;
using namespace sequence;

/* Kmeans** seeding
 * 1) Choose O(k) points uniformly at random
 * 2) Weight each of the sampled points by the number of data
 * 		points closer to it than any other sampled point.
 * 3) Recluster the weighted sample with kmeans.
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
void kmeansss (F seed[][D], F data[][D], F weights[], int n, int d, int k, 
						   DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr,
						   INPD inprod, F lfactor, int rseed, int rounds) {
	F p = lfactor * k * rounds / (F) n;

	// Initialize arrays
	for (int i = 0; i < n; i++) {
		include[i] = -1;
		weightsp[i] = 0;
	}

	// Sample each point independently with equal probability such that 
	// in the end we will have sampled rounds * lfactor * k points,
	// approximately.
	parallel_for (int y = 0; y < n; y++) {
		double pr = ((double) utils::hashInt (n * y + rseed) / INT_MAX);
		if (pr <= p) {
			include[y] = y;
		}
	}

	// Determine the number of sampled points
	int newn = filter (include, centers, n, notNeg<int>());

	parallel_for (int i1 = 0; i1 < newn; i1++) {
		for (int j = 0; j < d; j++) datap[i1][j] = data[centers[i1]][j];
	}
	
	// Weight each sampled point as described above.
	parallel_for (int ii = 0; ii < n; ii++) {
		F min = distancesqr (data[ii], datap[0]); int cl = 0;
		for (int j = 1; j < newn; j++) {
			F dd = distancesqr (data[ii], datap[j]);
			if (min > dd) {
				min = dd;
				cl = j;
			}
		}
		clusters[ii] = cl;
	}
	for (int i = 0; i < n; i++) {
		weightsp[clusters[i]] += 1;
	}

	// Recluster the points with kmeans.
	kmeanspp (seed, datap, weightsp, newn, d, k, distance, distancesqr, norm, 
						normsqr, inprod, rseed);
	lloyd <F, DST, DSTSQ, NRM, NRMSQ, INPD>
		(datap, weightsp, newn, d, k, seed, distance, distancesqr, norm, normsqr, inprod,
		 0.0, NULL, NULL, NULL, -1);
}

#endif
