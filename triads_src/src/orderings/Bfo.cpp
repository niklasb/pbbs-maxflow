/*
 * Bfo.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "Bfo.h"

Bfo::Bfo() :
		AbstractOrdering() {
}

Bfo::~Bfo() {
	// TODO Auto-generated destructor stub
}

int* Bfo::getPI(const Graph* const g) const {
	int i = 0;
	// number of nodes
	const int numOfNodes = g->n();
	// max degree
	const int maxDegree = g->maxDegree();
	// the buckets counting the number of nodes for the given degree
	int * const __restrict bucket = new int[maxDegree + 1];
	// the coreness
	int* const __restrict inducedDeg = new int[numOfNodes];
	// fill with zeros
	for (int i = 0; i <= maxDegree; ++i) {
		bucket[i] = 0;
	}
	// count the degrees
	int degree;
	for (; i < numOfNodes; ++i) {
		degree = g->degree(i);
		++bucket[degree];
		inducedDeg[i] = degree;
	}
	int first = 0;
	int size;
	for (int i = maxDegree; i >= 0; --i) {
		size = bucket[i];
		bucket[i] = first;
		first += size;
	}
	int* const __restrict pos = new int[numOfNodes];
	int* const __restrict pi = new int[numOfNodes];
	for (i = 0; i < numOfNodes; ++i) {
		degree = inducedDeg[i];
		int& position = pos[i];
		position = bucket[degree];
		pi[position] = i;
		++bucket[degree];
	}
	// since the end of bucket i is one pos earlier decrement it
	for (int i = maxDegree; i > 0; --i) {
		--bucket[i];
	}
//	bucket[0] = 0;
	int v, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	bucket[0] = 0;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			if (i < pos[w]) {
				int& inducedDegW = inducedDeg[w];
				// get the last node of the current bucket
				int& bucketDeg = bucket[inducedDegW];
				u = pi[bucketDeg];
				if (u != w) {
					int& posU = pos[u];
					int& posW = pos[w];
					posU = posW;
					posW = bucketDeg;
					pi[posU] = u;
					pi[posW] = w;
				}
				--bucketDeg;
				--inducedDegW;
			}
		}
	}
	delete[] inducedDeg;
	delete[] bucket;
	delete[] pos;
	return pi;
}

void Bfo::map(Graph* const g) const {
	int i = 0;
	// number of nodes
	const int numOfNodes = g->n();
	// max degree
	const int maxDegree = g->maxDegree();
	// the buckets counting the number of nodes for the given degree
	int * const __restrict bucket = new int[maxDegree + 1];
	// the coreness
	int* const __restrict inducedDeg = new int[numOfNodes];
	// fill with zeros
	for (int i = 0; i <= maxDegree; ++i) {
		bucket[i] = 0;
	}
	// count the degrees
	int degree;
	for (; i < numOfNodes; ++i) {
		degree = g->degree(i);
		++bucket[degree];
		inducedDeg[i] = degree;
	}
	int first = 0;
	int size;
	for (int i = maxDegree; i >= 0; --i) {
		size = bucket[i];
		bucket[i] = first;
		first += size;
	}
	int* const __restrict pos = new int[numOfNodes];
	int* const __restrict pi = new int[numOfNodes];
	for (i = 0; i < numOfNodes; ++i) {
		degree = inducedDeg[i];
		int& position = pos[i];
		position = bucket[degree];
		pi[position] = i;
		++bucket[degree];
	}
	// since the end of bucket i is one pos earlier decrement it
	for (int i = maxDegree; i > 0; --i) {
		--bucket[i];
	}
//	bucket[0] = 0;
	int v, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	bucket[0] = 0;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			if (i < pos[w]) {
				int& inducedDegW = inducedDeg[w];
				// get the last node of the current bucket
				int& bucketDeg = bucket[inducedDegW];
				u = pi[bucketDeg];
				if (u != w) {
					int& posU = pos[u];
					int& posW = pos[w];
					posU = posW;
					posW = bucketDeg;
					pi[posU] = u;
					pi[posW] = w;
				}
				--bucketDeg;
				--inducedDegW;
			} else {
				g->addOutEdge(w, i);
			}
		}
	}
	delete[] inducedDeg;
	delete[] bucket;
	delete[] pos;
	// set the ordering
	g->setOrdering(pi);
	g->addIncomingEdges();
	delete[] pi;
}
