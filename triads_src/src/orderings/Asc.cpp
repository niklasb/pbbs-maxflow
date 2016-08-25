/*
 * Asc.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "Asc.h"

Asc::Asc() :
		AbstractOrdering() {
}

Asc::~Asc() {
}

int* Asc::getPI(const Graph* const g) const {
	// number of nodes
	const int numOfNodes = g->n();
	// max degree
	const int maxDegree = g->maxDegree();
	// the buckets counting the number of nodes for the given degree
	int * const bucket = new int[maxDegree + 1];
	// fill with zeros
	for (int i = 0; i <= maxDegree; ++i) {
		bucket[i] = 0;
	}
	// count the degrees
	for (int i = 0; i < numOfNodes; ++i) {
		++bucket[g->degree(i)];
	}
	int first = 0;
	int size;
	for (int i = 0; i <= maxDegree; ++i) {
		size = bucket[i];
		bucket[i] = first;
		first += size;
	}
	int* const pi = new int[numOfNodes];
	int pos;
	for (int i = 0; i < numOfNodes; ++i) {
		const int degree = g->degree(i);
		pos = bucket[degree];
		pi[pos] = i;
		++bucket[degree];
	}
	delete[] bucket;
	return pi;
}

void Asc::map(Graph* const g) const {
	const int* adja;
	int pos;
	unsigned int j, size;
	const int numOfNodes = g->n();
	// get pi
	const int* const pi = this->getPI(g);
	const int* const piInv = AbstractOrdering::getInversePI(g, pi);
	// add the outedges
	for (int i = 1; i < numOfNodes; ++i) {
		pos = pi[i];
		adja = g->edges(pos);
		size = g->degree(pos);
		for (j = 0; j < size; ++j) {
			if (piInv[(pos = adja[j])] < i) {
				g->addOutEdge(pos, i);
			}
		}
	}
	delete[] piInv;
	// set the ordering
	g->setOrdering(pi);
	delete[] pi;
	// add the incoming edges
	g->addIncomingEdges();
}
