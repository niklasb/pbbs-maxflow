/* Author: Dongho Eugene Choi
 * Program: lloyd.h
 *
 * Description:
 * Lloyd's method for k-means. This is one of the most standard algorithms
 * used in solving the k-means problem (heuristically). It is a simple
 * greedy algorithm that comprises of two steps per iteration given a
 * starting "seed" of k centers. First we assign each data point to its 
 * closest center, and then we recalculate the centers by taking the centroid
 * of each cluster. Each part of the iteration is guaranteed to reduce the
 * score, or the sum of the square of the distances of each point to its 
 * closest center; clearly, assigning each point to its closest center reduces
 * the distance between each point and its closest center and so reduces the
 * score, and taking the centroid of each cluster reduces the sum of the 
 * squares of the distances between each point in the cluster to the center 
 * (this fact is easily shown using linear algebra or calculus) and so also 
 * reduces the total score. Beware, though; this algorithm only gives a locally
 * optimal result, and it has been shown that this local optimum can be
 * arbitrarily bad compared to the optimal solution depending on the beginning
 * seed.
 *
 * Problems:
 * 1) Sometimes, depending on input, a cluster may disappear. This is NOT a bug.
 * 		This is only the nature of the algorithm. If n is not >> k, then this 
 *		phenomenom occurs frequently.
 */

#ifndef LLOYD_H
#define LLOYD_H

#include "parallel.h"
#include "random.h"
#include "kmeansutils.h"
#include "gettime.h"
#include "utils.h"

template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
int lloyd (F data[][D], F weights[], int num, int dim, int k, F seed[][D],
					 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, INPD inprod, 
					 F error, F* scr, F* bfscr, int* steps, int numits) {
	// Initialize cluster assignments.
	parallel_for (int x = 0; x < num; x++) {
		F min = distancesqr (data[x], seed[0]);
		clusters[x] = 0;
		for (int i = 1; i < k; i++) {
			F d = distancesqr (data[x], seed[i]);
			if (d < min) { min = d; clusters[x] = i; }
		}
	}

	// Calculate and update the seeding score.
	F score = kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (bfscr != NULL) *bfscr = score; 
	
	bool done = false; int its = 0;

	// Run until the clusters converge.
	while (!done) {
		its++; 
		
		// Initialize the sum and cnt arrays to 0.
		for (int k2 = 0; k2 < k; k2++) {
			cnt[k2] = 0; for (int d1= 0; d1 < dim; d1++) sums[k2][d1] = 0.0;
		}
		
		// Update the clusters.
		parallel_for (int x = 0; x < num; x++) {
			F min = distancesqr (data[x], seed[0]);
			clusters[x] = 0;
			for (int i = 1; i < k; i++) {
				F d = distancesqr (data[x], seed[i]);
				if (d < min) { min = d; clusters[x] = i; }
			}
		}


		// Find the sum of the points in each cluster and the number of points 
		// in each cluster.
		for (int x2 = 0; x2 < num; x2++) {
			for (int d2 = 0; d2 < dim; d2++) 
				sums[clusters[x2]][d2] += data[x2][d2] * weights[x2];
			cnt[clusters[x2]] += weights[x2];
		}
		
		// Make sure no cluster became empty. If we know for sure the clusters 
		// won't disappear (running kmeans++ or kmeans|| initialization makes
		// the probability that a cluster will disappear almost 0, given that
		// the number of clusters is relatively small compared to the size of 
		// the input), then we can comment this part out to save computation.
		for (int kk = 0; kk < k; kk++) {
			if (cnt[kk] == 0) return -1;
		}

		// Find the mean of the points in each cluster
		for (int k3 = 0; k3 < k; k3++) {
			F temp[D];
			for (int d3 = 0; d3 < dim; d3++) {
				temp[d3] = seed[k3][d3];
				seed[k3][d3] = sums[k3][d3] / cnt[k3];
			}
			moved[k3] = distancesqr (temp, seed[k3]);
		}

		F mv = plusReduce (moved, k);
		if (mv == 0) done = true;
		if (its == numits) done = true;	
	
		// Recalculate the score.
		F nscore = 
			kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
		
		// If the score has not changed much (precisely, if the score has changed 
		// only by a small fraction < error), then if the centers have not yet
		// converged, we assume that convergence is near and the score is somewhat
		// close to the local optimum. 
		F e = (score - nscore)/nscore; score = nscore;
		if (!done && (e < error && -e < error)) done = true;
	} 
	
	// Caclculate and upate the final score and number of steps this algorithm
	// took.
	score = kmeansscore (data, clusters, weights, seed, dists, num, dim, k, distancesqr);
	if (scr != NULL) *scr = score;
	if (steps != NULL) *steps = its;
	
	return 0;
}

#endif
