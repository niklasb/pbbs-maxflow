/* Name: kmeansutils.h
 * Author: Dongho Eugene Choi
 * Includes basic arithmetic and methods for points
 */

#ifndef KMEANSUTILS_H
#define KMEANSUTILS_H

#include <iostream>
#include <stdlib.h>
#include "parallel.h"
#include "sequence.h"
#include "kmeansdefs.h"
using namespace sequence;
using namespace std;

template <class E>
struct notNeg {
	bool operator() (E a) { return a >= 0; }
};

/* Calculate the score of a data set given a set of cluster centers
 */
template <class F, class DSTSQ>
F kmeansscore (F data[][D], int clusters[], F weights[], F center[][D], 
								F dists[], int n, int d, int k, DSTSQ distancesqr) {
	parallel_for (int i = 0; i < n; i++)
		dists[i] = weights[i] * distancesqr (data[i], center[clusters[i]]);
	return plusReduce (dists, n);
}

/* Calculates the score of a data set given a set of cluster centers
 */
template <class F, class DSTSQ>
F naivescore (F data[][D], F weights[], F seed[][D], int n, int d, int k,
							DSTSQ distancesqr) {
	F sum = 0;
	for (int i = 0; i < n; i++) {
		F min = distancesqr (data[i], seed[0]);
		for (int j = 1; j < k; j++) {
			F dd = distancesqr (data[i], seed[j]);
			if (dd < min) min = dd;
		}
		sum += weights[i] * min;
	}
	return sum;
}

#endif
