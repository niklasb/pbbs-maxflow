/*
 * Sfo.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "Sfo.h"
#include "../network/Graph.h"

Sfo::Sfo() :
		AbstractOrdering() {
}

Sfo::~Sfo() {

}

int* Sfo::getPI(const Graph* const g) const {
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
	for (int i = 0; i <= maxDegree; ++i) {
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
	int v, inducedDegV, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		inducedDegV = inducedDeg[v];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			if (i < pos[w]) {
				int& inducedDegW = --inducedDeg[w];
				if (inducedDegW < inducedDegV && bucket[inducedDegW] < i) {
					bucket[inducedDegW] = i;
				}
				// since we decremented the degree of w we can increment the endpoint of the previous bucket
				++bucket[inducedDegW];
				// get the first node of the current bucket .. thats the node after the end index of the previous bucket
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
			}
		}
	}
	delete[] inducedDeg;
	delete[] bucket;
	delete[] pos;
	return pi;
}

void Sfo::map(Graph* const g) const {
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
	for (int i = 0; i <= maxDegree; ++i) {
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
	int v, inducedDegV, w, u;
	unsigned int neighSize, j;
	const int* __restrict neighborsV;
	bucket[0] = 0;
	for (i = 0; i < numOfNodes; ++i) {
		v = pi[i];
		inducedDegV = inducedDeg[v];
		neighborsV = g->edges(v);
		neighSize = g->degree(v);
		for (j = 0; j < neighSize; ++j) {
			w = neighborsV[j];
			if (i < pos[w]) {
				int& inducedDegW = --inducedDeg[w];
				if (inducedDegW < inducedDegV && bucket[inducedDegW] < i) {
					bucket[inducedDegW] = i;
				}
				// since we decremented the degree of w we can increment the endpoint of the previous bucket
				// ++bucket[inducedDegW];
				// get the first node of the current bucket .. thats the node after the end index of the previous bucket
				int& bucketDeg = ++bucket[inducedDegW];
				u = pi[bucketDeg];
				if (u != w) {
					int& posU = pos[u];
					int& posW = pos[w];
					posU = posW;
					posW = bucketDeg;
					pi[posU] = u;
					pi[posW] = w;
				}
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
