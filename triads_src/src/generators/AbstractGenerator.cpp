/*
 * AbstractGenerator.cpp
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#include "AbstractGenerator.h"
#include <stdlib.h>
#include "../network/IOGraph.h"

AbstractGenerator::AbstractGenerator() {
}

AbstractGenerator::~AbstractGenerator() {
}

IOGraph* AbstractGenerator::generateGraph(int numOfNodes) {
	IOGraph* g = generate(numOfNodes);
	makeConnected(g);
	permute(g);
	return g;
}

double AbstractGenerator::randDouble() const {
	int random;
	do {
		random = rand();
	} while (random == RAND_MAX);
	double val = ((double) random) / ((double) RAND_MAX);
	return val;
}

void AbstractGenerator::permute(IOGraph* g) {
	// permute the nodes and update their indices
	int* permutation = new int[g->n()];
	// fill with numbers 1 to n
	for (int i = 0; i < g->n(); ++i) {
		permutation[i] = i;
	}
	// do fisher-yales shuffle
	for (int i = g->n() - 1; i > 0; --i) {
		int j = randDouble() * (i + 1);
		int tmp = permutation[i];
		permutation[i] = permutation[j];
		permutation[j] = tmp;
	}
	g->permute(permutation);
	delete[] permutation;

	// permute every adjacency list
	for (int i = 0; i < g->n(); ++i) {
		vector<int>& adja = g->edges(i);
		for (int j = (int) adja.size() - 1; j > 0; --j) {
			int k = randDouble() * (j + 1);
			int tmp = adja[j];
			adja[j] = adja[k];
			adja[k] = tmp;
		}
	}
}

void AbstractGenerator::makeConnected(IOGraph* g) const {
	int* marked = new int[g->n()];
	// fill marked with 0
	for (int i = 0; i < g->n(); i++) {
		marked[i] = 0;
	}
	int i = 0;
	std::vector<int> vec;
	std::vector<int> rep;
	while (i < g->n()) {
		if (marked[i] == 0) {
			marked[i] = 1;
			vec.push_back(i);
			while (!vec.empty()) {
				int cur = vec.back();
				vec.pop_back();
				std::vector<int>& neighbors = g->edges(cur);
				for (int j = 0; j < (int) neighbors.size(); ++j) {
					int index = neighbors[j];
					if (marked[index] == 0) {
						marked[index] = 1;
						vec.push_back(index);
					}
				}
			}
			rep.push_back(i);
		}
		++i;
	}
	for (unsigned int i = 1; i < rep.size(); ++i) {
		g->addEdge(rep[i - 1], rep[i]);
	}
	delete[] marked;
}
