/*
 * Co.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "Co.h"

Co::Co() :
		AbstractOrdering() {
}

Co::~Co() {
}

int* Co::getPI(const Graph* const g) const {
	int i = 0;
	// number of nodes
	const int numOfNodes = g->n();
	// max degree
	const int maxDegree = g->maxDegree();
	// the buckets counting the number of nodes for the given degree
	int * const __restrict bucket = new int[maxDegree + 1];
	// the coreness
	int* const __restrict coreness = new int[numOfNodes];
	// fill with zeros
	for (int i = 0; i <= maxDegree; ++i) {
		bucket[i] = 0;
	}
	// count the degrees
	int degree;
	for (; i < numOfNodes; ++i) {
		degree = g->degree(i);
		++bucket[degree];
		coreness[i] = degree;
	}
	int first = 0;
	int size;
	for (int i = 0; i <= maxDegree; ++i) {
		size = bucket[i];
		bucket[i] = first;
		first += size;
	}
	int* const __restrict pos = new int[numOfNodes];
	int* const __restrict pi = new int[numOfNodes];
	for (i = 0; i < numOfNodes; ++i) {
		degree = coreness[i];
		int& position = pos[i];
		position = bucket[degree];
		pi[position] = i;
		++bucket[degree];
	}
	for (int i = maxDegree; i > 0; --i) {
		bucket[i] = bucket[i - 1];
	}
	int v, corenessV, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	bucket[0] = 0;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		corenessV = coreness[v];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			int& corenessW = coreness[w];
			if (corenessW > corenessV) {
				int& bucketDeg = bucket[corenessW];
				u = pi[bucketDeg];
				if (u != w) {
					int& posU = pos[u];
					int& posW = pos[w];
					posU = posW;
					posW = bucketDeg;
					pi[posU] = u;
					pi[posW] = w;
				}
				++bucketDeg;
				--corenessW;
			}
		}
	}
	delete[] coreness;
	delete[] bucket;
	delete[] pos;
	return pi;
}

void Co::map(Graph* const g) const {
	int i = 0;
	// number of nodes
	const int numOfNodes = g->n();
	// max degree
	const int maxDegree = g->maxDegree();
	// the buckets counting the number of nodes for the given degree
	int * const __restrict bucket = new int[maxDegree + 1];
	// the coreness
	int* const __restrict coreness = new int[numOfNodes];
	// fill with zeros
	for (int i = 0; i <= maxDegree; ++i) {
		bucket[i] = 0;
	}
	// count the degrees
	int degree;
	for (; i < numOfNodes; ++i) {
		degree = g->degree(i);
		++bucket[degree];
		coreness[i] = degree;
	}
	int first = 0;
	int size;
	for (int i = 0; i <= maxDegree; ++i) {
		size = bucket[i];
		bucket[i] = first;
		first += size;
	}
	int* const __restrict pos = new int[numOfNodes];
	int* const __restrict pi = new int[numOfNodes];
	for (i = 0; i < numOfNodes; ++i) {
		degree = coreness[i];
		int& position = pos[i];
		position = bucket[degree];
		pi[position] = i;
		++bucket[degree];
	}
	for (int i = maxDegree; i > 0; --i) {
		bucket[i] = bucket[i - 1];
	}
	int v, corenessV, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	bucket[0] = 0;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		corenessV = coreness[v];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			if (pos[w] < i) {
				g->addOutEdge(w, i);
			} else {
				int& corenessW = coreness[w];
				if (corenessW > corenessV) {
					int& bucketDeg = bucket[corenessW];
					u = pi[bucketDeg];
					if (u != w) {
						int& posU = pos[u];
						int& posW = pos[w];
						posU = posW;
						posW = bucketDeg;
						pi[posU] = u;
						pi[posW] = w;
					}
					++bucketDeg;
					--corenessW;
				}
			}
		}
	}
// clear the storage
	delete[] coreness;
	delete[] bucket;
	delete[] pos;
// set the ordering
	g->setOrdering(pi);
	g->addIncomingEdges();
	delete[] pi;
}
