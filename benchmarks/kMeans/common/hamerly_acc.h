/* Author: Dongho Eugene Choi
 * Program: hamerly_acc.h
 *
 * Description:
 * This is an accelerated version of Hamerly's method. A couple of tweaks have
 * been added to optimize Hamerly's method:
 * 
 * 1) At the expense of preprocessing the norms of the data point and centers,
 * 		the distane between two points is calculated using the following formula:
 *				d(x,y)^2 = |x| + |y| - 2 (x,y)
 * 		where |x| is the norm of x and (x,y) is the inner product of x and y. 
 * 		Compared to the standard, naive way of calculatin distance, where
 * 				d(x,y)^2  = sum_i (x_i - y_i)^2
 *		which takes about 3D-1 flops, where D is the dimension of the points, the
 *		method used here takes 2D+2 flops, where the inner product takes 2D-1 
 * 		flops. Furthermore, since the data points do not change, incorporating
 * 		the norms of the data points when only determing relative distances may
 *		not be necessary and may be added only when requiring absolute distances.
 *
 * 2) One may also note that in the cases where a clustering distance for data
 * 		point x must be recomputed, the original method scans through all the 
 * 		clusters. However, at the expense of sorting the cluster centers by 
 *		increasing norm, we do not need to visit every cluster. This method is
 * 		very effective when the number of clusters is large. For example, when
 * 		the number of clusters k is at least 50-100, the savings is significant
 * 		since the number of centers actually visited is only a small fraction of
 *		k for large k. The method relies on the Triangle Inequality and is based 
 *		on a technique in Wu and Lin's paper <Fast VQ Encoding by an Efficient
 * 		Kick-Out Condition>. For smaller k, we recommend using the original 
 * 		algorithm, for the overhead in precomputing norms as well as the fact 
 * 		that for smaller k's pretty much all of the centers must be visited for
 * 		each data point being considered will worsen execution time.
 */

#ifndef HAMERLY_ACC_H
#define HAMERLY_ACC_H

#include "parallel.h"
#include <algorithm>
#include <cmath>
#include "random.h"
#include "kmeansutils.h"
#include "utils.h"
#include "quickSort.h"

/* An operational struct used to determine the center with lesser norm
 * between two clusters centers.
 */
template <class F>
struct centerLess {
	bool operator() (int a, int b) {
		return (clnormsqr[a] < clnormsqr[b]);
	}
};

/* Takes the information on the lower bound of the secondary cluster distance
 * and the upper bound on the primary cluster distance and the current cluster
 * centers to update the centers.
 */
template <class F, class DST, class DSTSQ> 
inline void update_hamerly_acc
		(F data[][D], int x, F xnorm, F seed[][D], int clind[], F clnorm[], F upper[], 
		 F lower[], int clusters[], int dim, int k, DST distance, DSTSQ distancesqr) {
	// Estimate where to start the modified Wu-Lin acceleration. We start at the 
	// center with norm closest to the norm of the data point x.
	int beg = 0; int end = k;
	while (beg != end) {
		int mid = (beg + end) / 2; F cns = clnormsqr[clind[mid]];
		if (cns < xnormsqr[x]) beg = mid + 1;
		else end = mid;
	}
	int pos = beg; int pos2; if (pos > 0) pos2 = pos - 1; else pos2 = pos + 1;
	bool minexists = false; bool min2exists = false;
	F min, min2;

	// Search downard, meaning start from the center with norm closest to the norm
	// of x, and then work downwards until we reach the center with the smallest 
	// norm or we reach a center with sufficiently small norm such that any other
	// center with smaller norm could not possibly be closest to the data point x.
	int cl = clind[pos];
	for (int i = pos; i >= 0; i--) {
		int ind = clind[i];
		// Estimate the distance between the center and x
		F dist_est = clnormsqr[ind] - 2 * clnorm[ind] * xnorm + xnormsqr[x];

		// If the estimate is larger than the current value of the secondary distance,
		// and the norm of the center is smaller than the norm of x, then stop.
		if (minexists && min2exists && dist_est >= min2) {
			if (clnorm[ind] <= xnorm) break;
		}
		else {
			// Calculate the actual distance between the center and x
			F newd = distancesqr (data[x], seed[ind]);
			// Update appropriate values as necessary.
			if (minexists && newd < min) { 
				cl = ind; min2 = min; min = newd;
				min2exists = true;
			}
			else if (!minexists) {
				cl = ind; min = newd;
				minexists = true;
			}
			else if (min2exists && newd < min2) {
				min2 = newd;
			}
			else if (!min2exists) {
				min2 = newd; 
				min2exists = true;
			}
		}
	}

	// Similar to above, except we search upward.
	for (int i = pos + 1; i < k; i++) {
		int ind = clind[i];
		F dist_est = clnormsqr[ind] - 2 * clnorm[ind] * xnorm + xnormsqr[x];
		if (minexists && min2exists && dist_est >= min2) {
			if (clnorm[ind] >= xnorm) break;
		}
		else {
			F newd = distancesqr (data[x], seed[ind]);
			if (minexists && newd < min) { 
				cl = ind; min2 = min; min = newd;
				min2exists = true;
			}
			else if (!minexists) {
				cl = ind; min = newd;
				minexists = true;
			}
			else if (min2exists && newd < min2) {
				min2 = newd;
			}
			else if (!min2exists) {
				min2 = newd; 
				min2exists = true;
			}
		}
	}
	
	// Return the cluster closest to the data point x. Update the upper bound on
	// the primary cluster distance of x to the primary cluster distance, and
	// the lower bound on the secondary cluster distance to the secondary 
	// cluster distance.
	clusters[x] = cl; upper[x] = sqrt (min); lower[x] = sqrt (min2);
}

// The accelerated version of Hamerly's method.
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int hamerly_acc (F data[][D], F weights[], int num, int dim, int k, F seed[][D],
								 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, 
								 INPD inprod, F error, F* scr, F* bfscr, int* steps, 
								 int numits) {
	if (steps != NULL) *steps = 0;
	
	// Calculate and cache the norms and norm squares of each center.
	parallel_for (int a1 = 0; a1 < k; a1++) {
		clnormsqr[a1] = normsqr (seed[a1]);
		clnorm[a1] = sqrt (clnormsqr[a1]);
		clind[a1] = a1;
	}
	
	// Calculate and cache the norms and norm squares of the data points.
	parallel_for (int b1 = 0; b1 < num; b1++) {
		xnormsqr[b1] = normsqr (data[b1]);
		xnorm[b1] = sqrt (xnormsqr[b1]);
	}
	
	// Sort the centers in increasing order of norms.
	quickSort (clind, k, centerLess <F> ());

	// Initialize cluster assignments and bounds values
	parallel_for (int xx1 = 0; xx1 < num; xx1++) {
		update_hamerly_acc <F, DST, DSTSQ> 
			(data, xx1, xnorm[xx1], seed, clind, clnorm, ubound, lbound1, clusters, 
			 dim, k, distance, distancesqr);
	}
	
	// Calculate current score
	F score = 
		kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (bfscr != NULL) *bfscr = score; 
	bool done = false; int its = 0;

	while (!done) {
		its++;

		// Initialize sum and cnt arrays to 0.
		parallel_for (int k5 = 0; k5 < k; k5++) {
			cnt[k5] = 0; for (int d2 = 0; d2 < dim; d2++) sums[k5][d2] = 0.0;
		}

		// Recalculate and cache the norms and norm squares of the centers.
		// Don't use parallel_for - the time it takes to set up each processor
		// to do the following simple tasks is more costly than simply doing 
		// all the work in one go by one processor, especially if k is not
		// large.
		for (int a2 = 0; a2 < k; a2++) {
			clnormsqr[a2] = normsqr (seed[a2]);
			clnorm[a2] = sqrt (clnormsqr[a2]);
			clind[a2] = a2;
		}

		// Resort the centers by increasing order of norms.
		quickSort (clind, k, centerLess <F> ());
		
		// Compute the secondary distances of each center. Use for if the
		// number of clusters and dimensions are both relatively small, like
		// if both are at most 8.
		parallel_for (int k2 = 0; k2 < k; k2++) {
			F dd;
			if (k2 == 0) {
				dd = distance (seed[k2], seed[1]);
			}
			else {
				dd = distance (seed[k2], seed[0]);
			}
			for (int k3 = 0; k3 < k; k3++) {
				if (k3 != k2) {
					F d = distance (seed[k2], seed[k3]);
					if (d + EPSILON < dd) dd = d;
				}
			}
			sdist[k2] = dd;
		}

		// Update the centers
		parallel_for (int x3 = 0; x3 < num; x3++) {
			F m = max (sdist[clusters[x3]] / 2, lbound1[x3]);

			// If the upper bound of the cluster distance of point x is at most 
			// half the secondary distance of its closest center c(x), then x
			// will not change clusters, and we may avoid unnecessary compuation
			if (ubound[x3] > m) {
				ubound[x3] = distance (data[x3], seed[clusters[x3]]);

				// This is the same test as above, except that the upper bound of the
				// cluster distance of the point has been tightened. If this test fails
				// then	the point may have changed centers, meaning that we must re-
				// calculate the cluster assignment and tighten the bounds.

				if (ubound[x3] > m) {
					update_hamerly_acc (data, x3, xnorm[x3], seed, clind, clnorm, ubound,
															lbound1, clusters, dim, k, distance, distancesqr);
				}
			}
		}

		// Find the sum of the points in each cluster and the number of
		// points in each cluster
		for (int x4 = 0; x4 < num; x4++) {
			for (int d3 = 0; d3 < dim; d3++) 
				sums[clusters[x4]][d3] += data[x4][d3] * weights[x4];
			cnt[clusters[x4]] += weights[x4];
		}
		
		// Make sure no cluster became empty. If we know for sure the clusters
		// won't disappear (running kmeans++ or kmeans|| initialization makes 
		// the probability that a cluster will disappear almost 0), then we
		// can comment this part out to save computation.
		for (int kk = 0; kk < k; kk++) {
			if (cnt[kk] == 0) return -1;
		}

		// Find the mean of the points in each cluster and determine how much
		// each center moved. Use parallel_for instead if dimensions is greater than
		// 200.
		for (int k6 = 0; k6 < k; k6++) {
			F tempc[D];
			for (int d4 = 0; d4 < dim; d4++) {
				tempc[d4] = seed[k6][d4]; seed[k6][d4] = sums[k6][d4] / cnt[k6];
			}
			moved[k6] = distance (tempc, seed[k6]);
		}

		// Find the two centers that moved the most. Center r is the center
		// that moved the most and center rp is the one that moved the second most.
		int r = 0; int rp = 1;
		if (moved[r] + EPSILON < moved[rp]) { r = 1; rp = 0; }
		for (int x5 = 2; x5 < k; x5++) {
			if (moved[x5] > moved[r] + EPSILON) { rp = r; r = x5; }
			else if (moved[x5] > moved[rp] + EPSILON) { rp = x5; }
		}

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

		// Recalculate the score
		F nscore = 
			kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
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
