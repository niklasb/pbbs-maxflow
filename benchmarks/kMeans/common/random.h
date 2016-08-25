#ifndef RANDOM_H
#define RANDOM_H

#include <iostream>
#include <stdlib.h>
#include "utils.h"
//#include <cilk.h>
using namespace std;

// Generate k distinct random integers in [0,num]
// This algorithm is generally efficient when k << num
// (or k < num/2).
// O(n log(n/n-k)) expected work
// For k << n, O(k) expected work
inline void rands (int* list, int k, int n, int rseed) {
	int count = 0; // Keep track of how many numbers we have chosen so far
	int x = 0;
	for (int i = 0; i < k; i++) {
		int r; // the random number
		bool newnum;  // this holds the boolean flag representing whether or no
								  // r has already been chosen (false) or not (true)
		do {
			//r = rand() % n; // generate (pseudo)random integer from [0,n)
			r = utils::hashInt (x + rseed) % n;
			newnum = true;
			for (int j = 0; j < count; j++) {
				if (r == list[j]) {
					newnum = false; break; // if r appeared before, newnum is false
				}
			}
			x++;
		} while (!newnum);
		list[count] = r;
		count++;
	}
}

#endif
