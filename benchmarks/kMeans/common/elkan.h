/* Author: Dongho Eugene Choi
 * Program: elkan.h
 *
 * Description:
 * Elkan's method for k-means. This algorithm accelerates the standard Lloyd's
 * iteration by keeping track of (1) ubound bounds on the distances between each
 * data point and its closest center, which I will call the cluster distance
 * of the data point, (2) lbound bounds on the distances between each point 
 * and each center, and (3) distances between each center and the closest other
 * center to it, which I will call the secondary distance of each center. By
 * using the Triangle Inequality and these cached statistics, we can reduce the
 * number of distance calculations. For example, if data x is closest to center
 * c, and there is an other center b, and if d(b,c) >= 2 * d(x,c), then by 
 * the Triangle Inequality, it must be the case that d(x,b) >= d(x,c), so 
 * that we do not need to computer d(x,c). Thus, if the ubound bound on the 
 * cluster distance of x is at most half the secondary distance of its current
 * cluster, then the data point x has not changed clusters, and thus we can
 * skip all related distance computations. Similarly, if the ubound bound on
 * the cluster distance of x is at most the lbound bound of the distance between
 * x and some other cluster c, then we need not compute the distance between
 * x and c. In later iterations of Lloyd, when clusters begin to settle and
 * most points do not change clusters, this reduction will bring a huge savings
 * in terms of computation and time. Experimentally, this method seems to be
 * one of the algorithms that can handle high-dimensional points relatively
 * well.
 */

#ifndef ELKAN_H
#define ELKAN_H

#include "parallel.h"
#include <algorithm>
#include <cmath>
#include "random.h"
#include "kmeansutils.h"
#include "utils.h"
#include "quickSort.h"

/* The standard Lloyd's algorithm with Elkan's optimization used to find
 * a locally optimum solution to k-means given n input points
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int elkan (F data[][D], F weights[], int num, int dim, int k, F seed[][D], 
					 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, 
					 INPD inprod, F error, F* scr, F* bfscr, int* steps, int numits) {
	// Initialize the ubound and lbound bounds
	parallel_for (int q1 = 0; q1 < num; q1++) {
		for (int q2 = 0; q2 < k; q2++) lbound[q1][q2] = 0;
		ubound[q1] = 0;
	}
	
	// Find distance between centers and for each center find the
	// distance to the closest other center
	parallel_for (int w3 = 0; w3 < k; w3++) {
		F min;
		if (w3 == 0) {
			cdists[w3][1] = distance (seed[w3], seed[1]);
			min = cdists[w3][1];
		}
		else {
			cdists[w3][0] = distance (seed[w3], seed[0]);
			min = cdists[w3][0];
		}
		for (int w4 = 0; w4 < k; w4++) {
			if (w3 != w4) {
				cdists[w3][w4] = distance (seed[w3], seed[w4]);	
				if (min > cdists[w3][w4]) {
						min = cdists[w3][w4];
				}
			}
		}
		sdist[w3] = min;
	}

	// Assign each data point to its closest center. Update the ubound bounds
	// to the distance between the point and the closest center, and update
	// the lbound bounds on the distance between each point and each center
	// to the actual distance whenever it is actually calculated.
	parallel_for (int w1 = 0; w1 < num; w1++) {
		int cl = 0; F min = distance (data[w1], seed[0]); lbound[w1][0] = min;
		for (int i = 1; i < k; i++) {
			if (cdists[cl][i] < 2 * min) {
				lbound[w1][i] = distance (data[w1], seed[i]);
				if (min > lbound[w1][i] + EPSILON) { min = lbound[w1][i]; cl = i; }
			}
		}
		ubound[w1] = min;
		clusters[w1] = cl;
	}
	
	// Calculate and update the seeding score.
	F score = 
		kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (bfscr != NULL) *bfscr = score;
	bool done = false; int its = 0; 
	
	// Run until the clusters stop moving.
	while (!done) {
		its++;

		// Initialize sum and cnt arrays to 0.
		parallel_for (int k5 = 0; k5 < k; k5++) {
			cnt[k5] = 0; for (int d2 = 0; d2 < dim; d2++) sum[k5][d2] = 0.0;
		}
	
		// Compute intercentroid distances and for each center find the
		// distance to the closest other center. Call this distance the
		// secondary distance from the center.
		parallel_for (int w3 = 0; w3 < k; w3++) {
			F min;
			if (w3 == 0) {
				cdists[w3][1] = distance (seed[w3], seed[1]);
				min = cdists[w3][1];
			}
			else {
				cdists[w3][0] = distance (seed[w3], seed[0]);
				min = cdists[w3][0];
			}
			for (int w4 = 0; w4 < k; w4++) {
				if (w3 != w4) {
					cdists[w3][w4] = distance (seed[w3], seed[w4]);
					if (min > cdists[w3][w4]) {
						min = cdists[w3][w4];
					}
				}
			}
			sdist[w3] = min;
		}
		
		// Update the clusters
		parallel_for (int x1 = 0; x1 < num; x1++) {
			bool outofdate = true;
			// If the ubound bound on the distance of the data point to its closest center
			// is less than half the secondary distance of that center, then the data point
			// cannot change clusters
			if (ubound[x1] > 0.5 * sdist[clusters[x1]]) {
				for (int c = 0; c < k; c++) {
					// If the ubound bound is greater than the lbound bound of the distance between
					// the data point and cluster c, or if the ubound bound is greater than half 
					// the distance between the currently assigned center and center c, then we 
					// must recompute d(x,c).
					if (c != clusters[x1] && ubound[x1] > lbound[x1][c]
							&& ubound[x1] > 0.5 * cdists[clusters[x1]][c]) {
						// If uppder bound is out of date, update it.
						F dst;
						if (outofdate) {
							dst = distance (data[x1], seed[clusters[x1]]);
							ubound[x1] = dst;
							outofdate = false;
						}
						else {
							dst = ubound[x1];
						}					
						
						// Reassign data point to new closest center. We only need to compute distances
						// if the distance between the data point and its closest center is
						// greater than the lbound bound on the distance between the data point and 
						// center c, or half the distance between the closest center and center c.
						if (dst > lbound[x1][c] &&
								dst > 0.5 * cdists[clusters[x1]][c]) {
							F dd = distance (data[x1], seed[c]); lbound[x1][c] = dd;
							if (dd < dst) { clusters[x1] = c; outofdate = true; }
						}
					}
				}
			}
		}

		// Find the sum of the points in each cluster and the number of
		// points in each cluster
		for (int x4 = 0; x4 < num; x4++) {
			for (int d3 = 0; d3 < dim; d3++) 
				sum[clusters[x4]][d3] += data[x4][d3] * weights[x4];
			cnt[clusters[x4]] += weights[x4];
		}
		
		// Make sure no cluster became empty. If we know for sure the
		// clusters won't disappear (running kmeans++ or kmeans||
		// initialization makes the probability that a cluster will 
		// disappear almost 0), then we can comment this part out to
		// save computation.
		for (int kk = 0; kk < k; kk++) {
			if (cnt[kk] == 0) return -1;
		}

		// Find the mean of the points in each new cluster, and determine how
		// much each center moved.
		for (int k6 = 0; k6 < k; k6++) {
			F tempc[D];
			for (int d4 = 0; d4 < dim; d4++) {
				tempc[d4] = seed[k6][d4];
				seed[k6][d4] = sum[k6][d4] / cnt[k6];
			}
			moved[k6] = distance (tempc, seed[k6]);
		}

		// Each lbound bound decreases by how much each corresponding center
		// moved, and each ubound bound increases by how much each 
		// corresponding center moved.
		parallel_for (int e1 = 0; e1 < num; e1++) {
			for (int i = 0; i < k; i++) lbound[e1][i] = max (0.0, lbound[e1][i] - moved[i]);
			ubound[e1] += moved[clusters[e1]];
		}

		// If no centers moved, then the clusters converged, and we are done
		F mv = plusReduce (moved, k);
		if (mv == 0) done = true;	
		if (its == numits) done = true;

		// Recalculate score. 
		F nscore = 
			kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);

		// If the change in score is small enough, then we assume that the
		// clusters are very close to converging and that the score is close
		// to the final score, and so we stop the iterations prematurely.
		F e = (score - nscore)/nscore; score = nscore;
		if (!done && (e < error && -e < error)) done = true;
	}

	// Calculate and update the final score and number of iterations this 
	// algorithm took.
	score = kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (scr != NULL) *scr = score;
	if (steps != NULL) *steps = its;
	
	return 0;
}

#endif
