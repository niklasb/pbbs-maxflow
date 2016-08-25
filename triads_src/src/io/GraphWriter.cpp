/*
 * GraphWriter.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "GraphWriter.h"

#include "../network/IOGraph.h"

using namespace std;

GraphWriter::GraphWriter() {
}

GraphWriter::~GraphWriter() {
}

bool GraphWriter::writeGraph(const IOGraph* const g, string& fileName,
		bool newStyle) {
	// open the writer
	writer.open(fileName.c_str());
	if (!writer.is_open()) {
		throw ios_base::failure("cannot open " + fileName);
	}
// first line is the number of nodes
	int nodeCount = g->n();
	writer << nodeCount << endl;
	nodeCount = nodeCount - 1;
// if we use the new style we have to add the degrees of the nodes
	if (newStyle) {
		for (int i = 0; i <= nodeCount; ++i) {
			writer << i << "\t" << g->degree(i) << endl;
		}
	}
	// permute the nodes and update their indices
	int* permutation = new int[g->n()];
	// fill with numbers 1 to n
	for (int i = 0; i < g->n(); ++i) {
		permutation[i] = i;
	}
	// do fisher-yales shuffle
	for (int i = g->n() - 1; i > 0; --i) {
		int random;
		do {
			random = rand();
		} while (random == RAND_MAX);
		double val = ((double) random) / ((double) RAND_MAX);
		int j = val * (i + 1);
		int tmp = permutation[i];
		permutation[i] = permutation[j];
		permutation[j] = tmp;
	}
	// write the adjacency list
	unsigned long m = g->m();
	for (int i = 0; i <= nodeCount; ++i) {
		int pos = permutation[i];
		vector<int>& adja = g->edges(pos);
		for (unsigned int j = 0; j < adja.size(); ++j) {
			if (adja[j] > pos) {
				writer << pos << "\t" << adja[j];
				if (--m > 0) {
					writer << endl;
				}
			}
		}
	}
	delete[] permutation;
// flush and close
	writer.flush();
	writer.close();
	return true;
}
