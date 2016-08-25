#ifndef KMEANSPP_H
#define KMEANSPP_H

#include "sequence.h"
#include "kmeansdefs.h"
#include "utils.h"
using namespace std;
using namespace sequence;

/* Kmeans++ seeding
 * 1) Choose one center uniformly at random from the data set
 * 2) Repeat the following k-1 times
 *		a) Choose a point from the remaining data points with
 *			 randomly according the the distribution p(x) = D(x)/T, 
 *			 where D(x) is the squared distance from point x to the 
 * 			 current set of centers, and T is the current score.
 * 		b) Updat all necessary values
 */
template <class F, class DST, class DSTSQ, class NRM, class NRMSQ, class INPD>
void kmeanspp (F seed[][D], F data[][D], F weights[], int n, int d, int k, 
							 DST distance, DSTSQ distancsqr, NRM norm, NRMSQ normsqr,
							 INPD inprod, int rseed) {	
	// Sample first center uniformly at random.
	int c = utils::hashInt (rseed) % n;
	for (int d1 = 0; d1 < d; d1++) seed[0][d1] = data[c][d1];
	parallel_for (int x1 = 0; x1 < n; x1++) {
		pd[x1] = distance (data[x1], seed[0]);
		pd[x1] *= weights[x1] * pd[x1];
	}

	// Choose k-1 more centers with D^2 sampling
	for (int k1 = 1; k1 < k; k1++) {
		// Create the probability distribution, scaled by tot
		F tot = plusScan (pd, p_sec, n); p_sec[n] = tot;
		F r = ((F) utils::hashInt (k1 + rseed) / INT_MAX) * tot;

		// Find the data point corresponding to the real value r picked
		// randomly according to the described distribution
		parallel_for (int x2 = 1; x2 <= n; x2++) {
			if (r < p_sec[x2] && r > p_sec[x2 - 1]) {
				for (int d2 = 0; d2 < d; d2++) seed[k1][d2] = data[x2 - 1][d2];
			}
		}
		
		// Recalculate the distances from each point to its new centers
		parallel_for (int x3 = 0; x3 < n; x3++) {
			// We only need to compare the current distance and the 
			// previous distance, and update accordingly
			F dd = distance (data[x3], seed[k1]);
			dd *= weights[x3] * dd;
			if (dd < pd[x3]) pd[x3] = dd;
		}
	}
}

#endif
