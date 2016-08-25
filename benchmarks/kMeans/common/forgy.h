#ifndef FORGY_H
#define FORGY_H

#include "random.h"
#include "kmeansdefs.h"
#include "utils.h"
using namespace std;

/* Forgy seeding
 * Simply pick k distinct centers uniformly at random from the data set.
 */
template <class F>
void forgy (F seed[][D], F data[][D], int n, int d, int k, int rseed) {
	rands (randoms, k, n, rseed);
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < d; j++) seed[i][j] = data[randoms[i]][j];
	}
}

#endif
