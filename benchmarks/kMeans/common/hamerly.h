/* Author: Dongho Eugene Choi
 * Program: hamerly.h
 *
 * Description:
 * Hamerly's method for k-means. This algorithm accelerates the standard 
 * Lloyd's iteration by keeping track of (1) upper bounds on the distances
 * between each data point and its closest center, which I will call the 
 * cluster distance of the data point, (2) lower bounds on the dsitances
 * between each data point and its second closest center, which I will call
 * the secondary cluster distance of the data point, and (3) the distances
 * between each center and the closest other center, whcih I wil call the 
 * secondary distance of the center. By using the Triangle Inequality and
 * these cached statistics, we can reduce the number of distance calcul-
 * ations. Here is an example: if a data point x is closest to some center
 * c, and there is an other center b, and if d(b,c) >= 2 * d(x,c), then by 
 * the Triangle Inequality, it must be the case that d(x,b) >= d(x,c), so 
 * that we do not need to computer d(x,c). Thus, if the upper bound on the 
 * cluster distance of x is at most half the secondary distance of its current
 * cluster, then the data point x has not changed clusters, and thus we can
 * skip all related distance computations. Similarly, if the upper bound on
 * the cluster distance of x is at most the secondary cluster distance, then
 * x will not change clusters, and any related computation can be avoided as
 * well. In later iterations of Lloyd, when clusters begin to settle and most 
 * points do not change clusters, this reduction will bring a huge savings
 * in terms of computation and time. Experimentally, this method seems to be
 * one of the algorithms that can handle medium-dimensional points relatively
 * well.
 */

#ifndef HAMERLY_H
#define HAMERLY_H

#include "parallel.h"
#include <algorithm>
#include <cmath>
#include "random.h"
#include "kmeansutils.h"
#include "utils.h"
#include "quickSort.h"

/* The standard Lloyd algorithm with the Hamerly optimization used to 
 * find a locally optimal solution to k-means given n input points
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int hamerly (F data[][D], F weights[], int num, int dim, int k, F seed[][D], 
						 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsq, 
						 INPD inprod, F error, F* scr, F* bfscr, int* steps, int numits) {
	// Initialize cluster assignments and bounds values. Set each upper 
	// bound to the cluster distance, and each lower bound to the secondary
	// cluster distance. Assign each point to its closest center.
	parallel_for (int x1 = 0; x1 < num; x1++) {
		clusters[x1] = 0; 
		ubound[x1] = distance (seed[0], data[x1]); 
		lbound1[x1] = distance (seed[1], data[x1]);
		if (ubound[x1] > lbound1[x1] + EPSILON) {
			F temp = ubound[x1]; ubound[x1] = lbound1[x1]; lbound1[x1] = temp;
			clusters[x1] = 1;
		}
		for (int k1 = 2; k1 < k; k1++) {
			F d = distance (seed[k1], data[x1]);
			if (d + EPSILON < ubound[x1]) {
				lbound1[x1] = ubound[x1]; ubound[x1] = d; clusters[x1] = k1;
			}
			else if (d + EPSILON < lbound1[x1]) lbound1[x1] = d;
		}
	}

	cout << "asd\n";
	
	// Calculate and update the seeding score
	F score = 
		kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (bfscr != NULL) *bfscr = score; 

	bool done = false; int its = 0;

	// Run until the clusters stop moving.
	while (!done) {
		its++;

		// Initialize the sum and cnt arrays to 0.
		for (int k5 = 0; k5 < k; k5++) {
			cnt[k5] = 0; for (int d2 = 0; d2 < dim; d2++) sums[k5][d2] = 0.0;
		}
	
		// Compute intercentroid distances and find the secodnary distances
		// for each center. Use for if k and d are small, i.e. at most 8.
		parallel_for (int k2 = 0; k2 < k; k2++) {
			if (k2 == 0) sdist[k2] = distance (seed[k2], seed[1]);
			else sdist[k2] = distance (seed[k2], seed[0]);

			for (int k3 = 0; k3 < k; k3++) {
				if (k3 != k2) {
					F d = distance (seed[k2], seed[k3]);
					if (d < sdist[k2]) sdist[k2] = d;
				}
			}
		}
		
		// Update the clusters
		parallel_for (int x3 = 0; x3 < num; x3++) {
			F m = max (sdist[clusters[x3]] / 2, lbound1[x3]);

			// If the upper bound of the cluster distance of point x is at most 
			// half the secondary distance of its closest center c(x), then x
			// will not change clusters, and we may avoid unnecessary compuation.
			if (ubound[x3] > m) {
				ubound[x3] = distance (data[x3], seed[clusters[x3]]);	
				
				// This is the same test as above, except that the upper bound of the
				// cluster distance of the point has been tightened. If this test fails
				// then	the point may have changed centers, meaning that we must re-
				// calculate the cluster assignment and tighten the bounds.
				if (ubound[x3] > m) {
					clusters[x3] = 0; 
					ubound[x3] = distance (seed[0], data[x3]);
					lbound1[x3] = distance (seed[1], data[x3]);
					if (ubound[x3] > lbound1[x3] + EPSILON) {
						F temp = ubound[x3]; ubound[x3] = lbound1[x3]; lbound1[x3] = temp;
						clusters[x3] = 1;
					}
					for (int k4 = 2; k4 < k; k4++) {
						F d = distance (seed[k4], data[x3]);
						if (d + EPSILON < ubound[x3]) {
							lbound1[x3] = ubound[x3]; ubound[x3] = d; clusters[x3] = k4;
						}
						else if (d + EPSILON < lbound1[x3]) lbound1[x3] = d;
					}
				}
			}
		}

		// Find the sum of the points in each cluster and the number of
		// points in each cluster.
		for (int x4 = 0; x4 < num; x4++) {
			for (int d3 = 0; d3 < dim; d3++) 
				sums[clusters[x4]][d3] += data[x4][d3] * weights[x4];
			cnt[clusters[x4]] += weights[x4];
		}
		
		// Make sure no cluster became empty. If we know for sure the clusters 
		// won't disappear (running kmeans++ or kmeans|| initialization makes 
		// the probability that a cluster will disappear almost 0), then we can 
		// comment this part out to save computation.
		for (int kk = 0; kk < k; kk++) {
			if (cnt[kk] == 0) return -1;
		}

		// Find the mean of the points in each cluster, and determine how much 
		// each center moved. Use parallel_for if dimensions is greater than 200.
		for (int k6 = 0; k6 < k; k6++) {
			F tempc[D];
			for (int d4 = 0; d4 < dim; d4++) {
				tempc[d4] = seed[k6][d4];
				seed[k6][d4] = sums[k6][d4] / cnt[k6];
			}
			moved[k6] = distance (tempc, seed[k6]);
		}

		// Find the two centers that moved the most. Center r is the center that
		// moved the most, and center rp is the one that moved the second most.
		int r = 0; int rp = 1;
		if (moved[r] < moved[rp]) { r = 1; rp = 0; }
		for (int x5 = 2; x5 < k; x5++) {
			if (moved[x5] > moved[r]) { rp = r; r = x5; }
			else if (moved[x5] > moved[rp]) { rp = x5; }
		}

		// Increase each upper bound by the distance the corresponding center moved. 
		// Decrease the lower bound by the largest distance moved. If the center 
		// moved the largest distance, then we can decrease the lower bound instead 
		// by the second largest distance the centers moved.
		parallel_for (int x6 = 0; x6 < num; x6++) {
			ubound[x6] += moved[clusters[x6]];
			if (r == clusters[x6]) lbound1[x6] -= moved[rp];
			else lbound1[x6] -= moved[r];
		}
		
		// If no centers moved, then the clusters converged, and we are done.
		F mv = plusReduce (moved, k);
		if (mv == 0) done = true;
		if (its == numits) done = true;
		
		// Recalculate the score,
		F nscore = 
			kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
		F e = (score - nscore)/nscore; score = nscore;
		if (!done && (e < error && -e < error)) done = true;
	}

	// Calculate and update the final score and number of iterations this algorithm
	// took.
	score = kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (scr != NULL) *scr = score;
	if (steps != NULL) *steps = its;

	return 0;
}

#endif
