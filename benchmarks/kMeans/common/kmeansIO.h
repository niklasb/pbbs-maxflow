#ifndef KMEANSIO_H
#define KMEANSIO_H

#include <iostream>
#include <fstream>

/* Read the input from the file with the given filename
 * and initialize the data table with the input points
 * and fill info[0] with the number of points and info[1]
 * with the dimension of the points 
 */
template <class F>
void getinput (char* filename, F data[][D], F weights[], int info[2]) {
	string line;
	int num = 0, dim = 0;
	ifstream input (filename);
	
	// The first two number in the file should designate the number of
	// point in the dataset and the dimension of the data
	input >> num >> dim;
	
	// Read in num * dim numbers after that. The next dim numbers represent
	// the first data point, the dim numbers after that represent the second
	// data point, etc. The numbers are to be delimited by any whitespace.
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < dim; j++) {
			F n;
			input >> n;
			data[i][j] = n;
		}
		// By default assign a weight of 1 to each data point
		weights[i] = 1;
	}
	input.close();
	info[0] = num; info[1] = dim;
}

inline void printinfo (int num, int dim, int k) {
	cout << "Number of points: " << num << "\n";
	cout << "Dimension of points: " << dim << "\n";
	cout << "Number of clusters: " << k << "\n";
}

#endif
