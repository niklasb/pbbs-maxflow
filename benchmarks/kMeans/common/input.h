#ifndef INPUT_H
#define INPUT_H

#include <fstream>
using namespace std;

/* Input seeding
 * Read k centers from an input file
 */
template <class F>
void input (F seed[][D], char* seedfile, int d, int k) {
	ifstream in (seedfile);
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < d; j++) {
			in >> seed[i][j];
		}
	}
	in.close();
}

#endif
