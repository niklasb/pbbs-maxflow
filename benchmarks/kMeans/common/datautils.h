#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <iostream>
#include <algorithm>
#include <cmath>
using namespace std;

// A namespace for Euclidean distances, norms, and inner products
namespace euclid {

	template <class F>
	struct distance {
		int dim;
		distance (int d) : dim (d) {}
		F operator() (F a[], F b[]) {
			F sum = 0;
			for (int i = 0; i < dim; i++) {
				F dd = a[i] - b[i];
				sum += dd * dd;
			}
			return sqrt (sum);
		}
	};	

	template <class F>
	struct distanceSquare {
		int dim;
		distanceSquare (int d) : dim (d) {}
		F operator() (F a[], F b[]) {
			F sum = 0;
			for (int i = 0; i < dim; i++) {
				sum += (a[i] - b[i]) * (a[i] - b[i]);
			}
			return sum;
		}
	};	

	template <class F>
	struct norm {
		int dim;
		norm (int d) : dim (d) {}
		F operator() (F a[]) {
			F sum = 0; 
			for (int i = 0; i < dim; i++) {
				sum += a[i] * a[i];
			}
			return sqrt (sum);
		}
	};

	template <class F>
	struct normSquare {
		int dim;
		normSquare (int d) : dim (d) {}
		F operator() (F a[]) {
			F sum = 0; 
			for (int i = 0; i < dim; i++) {
				sum += a[i] * a[i];
			}
			return sum;
		}
	};


	template <class F>
	struct innerProduct {
		int dim;
		innerProduct (int d) : dim (d) {}
		F operator() (F a[], F b[]) {
			F sum = 0;
			for (int i = 0; i < dim; i++) {
				sum += a[i] * b[i];
			}
			return sum;
		}
	};
};

#endif
