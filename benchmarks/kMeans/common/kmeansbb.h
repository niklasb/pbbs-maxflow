#ifndef KMEANSBB_H
#define KMEANSBB_H

#include "parallel.h"
#include "kmeansdefs.h"
#include <cmath>
#include "sequence.h"
#include "lloyd.h"
#include "kmeanspp.h"
#include "kmeansutils.h"
using namespace std;
using namespace sequence;

/* Kmeans|| seeding
 * 1) Choose one center uniformly at random from the data set
 * 2) Repeat the following some number of times:
 *		a) Sample each data point randomly with some probability 
 *			 proportional to the square distance of the point
 *			 to the centers. Add the points to the centers
 * 3) Weight each sampled point by the number of data points 
 *		closer to it that any other sampled point.
 * 4) With that weighted sample from step 3, run K-means on the 
 *		sample set to recluster the sampled points. The resulting 
 *		cluster centers are the initial seeds
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
void kmeansbb (F seed[][D], F data[][D], F weights[], int n, int d, int k, 
							 DST distance, DSTSQ distancesqr, NRM norm, NRMSQ normsqr, 
							 INPD inprod, F lfactor, int rseed, int rounds)  {
	int count = 0; F total;
	parallel_for (int i = 0; i < n; i++) { include[i] = -1; newinclude[i] = -1; }

	// Sample first center uniformly at random from data
	int firstcenter = utils::hashInt (rseed) % n; 
	include[firstcenter] = firstcenter;
	newinclude[firstcenter] = firstcenter;

	// Calculate score and distances and cluster assignments
	parallel_for (int x1 = 0; x1 < n; x1++) {
		dsqr[x1] = distance (data[x1], data[firstcenter]);
		dsqr[x1] *= weights[x1] * dsqr[x1];
	}
	total = plusReduce (dsqr, n); count = 1;
	
	// Iteratively add about lfactor * k centrs
	for (int r1 = 0; r1 < rounds; r1++) {
		parallel_for (int x2 = 0; x2 < n; x2++) newinclude[x2] = -1;
		
		// Sample each point with probability propotional to the square of
		// its distance to the centers
		parallel_for (int x3 = 0; x3 < n; x3++) {
			F r = ((F) (utils::hashInt (rseed + r1 * n + x3)) / INT_MAX);
			if (r < dsqr[x3] * (lfactor * k) / total) {
				include[x3] = x3; newinclude[x3] = x3;
			}
		}

		// Determine how many new centers were added
		int newcount = filter (newinclude, centers, n, notNeg<int>());
	
		// Recalculate scores and distances
		parallel_for (int x4 = 0; x4 < n; x4++) {
			// Only need to calculate distances from each point to the new
			// centers and compare them with the previous distance, and 
			// update accordingly
			F temp = distance (data[x4], data[centers[0]]);
			temp *= weights[x4] * temp;
			for (int y1 = 1; y1 < newcount; y1++) {
				F ds = distance (data[x4], data[centers[y1]]);
				ds *= weights[x4] * ds;
				if (temp > ds) temp = ds;
			}
			if (temp < dsqr[x4]) dsqr[x4] = temp;
		}
		total = plusReduce (dsqr, n); count += newcount;
	}

	// Get the centers; only the non-negative values of include are centers
	filter (include, centers, n, notNeg<int>());
	
	// Determine the weights of each center. The weights of each center is
	// the number of data points for which it is the closest center to
	parallel_for (int y2 = 0; y2 < count; y2++) weights[y2] = 0; 
	int clust[N];
	parallel_for (int x5 = 0; x5 < n; x5++) {
		F min = distance (data[x5], data[centers[0]]); min *= min;
		clust[x5] = 0;
		for (int y3 = 1; y3 < count; y3++) {
			F ds = distance (data[x5], data[centers[y3]]);
			ds *= ds;
			if (ds < min) {
				clust[x5] = y3; min = ds;
			}
		}
	}
	for (int x6 = 0; x6 < n; x6++) weightsp[clust[x6]]++;
	
	// Initialize table of new data points consisting of the centers	
	parallel_for (int y4 = 0; y4 < count; y4++) {
		for (int d1 = 0; d1 < d; d1++) {
			datap[y4][d1] = data[centers[y4]][d1];
		}
	}
	//cout << "Entering kmeans++ reclustering.\n";
	
	// Recluster the weighted center points. Here, I am using k-means++ for seeding
	// and Lloyd's method for the iterations.
	kmeanspp (seed, datap, weightsp, count, d, k, distance, distancesqr, norm, 
						normsqr, inprod, rseed);
	lloyd <F, DST, DSTSQ, NRM, NRMSQ, INPD> 
		(datap, weightsp, count, d, k, seed, distance, distancesqr, norm, normsqr, inprod, 
		 0.0, NULL, NULL, NULL, -1);
}

#endif
