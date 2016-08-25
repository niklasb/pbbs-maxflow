/*
 * EdgeIterator.cpp
 *
 *  Created on: 02.06.2013
 *      Author: ortmann
 */

#include "EdgeIterator.h"
#include <algorithm>
#include "../../network/Graph.h"

EdgeIterator::EdgeIterator(string name) :
		AbstractTriadAlgorithm(name) {
}

EdgeIterator::~EdgeIterator() {
}

void EdgeIterator::prepare(Graph* const g) {
	const int numOfNodes = g->n();
	int* adja;
	for (int i = 0; i < numOfNodes; ++i) {
		adja = g->modifiableEdges(i);
		// sort adjacency list in ascending order
		sort(adja, adja + g->degree(i));
	}
}

void EdgeIterator::countTriads(Graph* const g) {
	const int numOfNodes = g->n();
	int t1degree, t3, t3Degree;
	const int* t1Edges;
	const int* t1OuterEdgeIter;
	const int* t1EdgeIter;
	const int* t1EdgeIterEnd;
	const int* t3EdgeIter;
	const int* t3EdgeIterEnd;
	for (int i = 0; i < numOfNodes; ++i) {
		t1degree = g->degree(i);
		if (t1degree < 2) {
			continue;
		}
		t1Edges = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t1OuterEdgeIter = t1Edges;
		t1EdgeIterEnd = t1Edges + t1degree;
		do {
			t3 = *t1OuterEdgeIter;
			t3Degree = g->degree(t3);
			if (t3Degree < 2) {
				continue;
			}
			t3EdgeIter = g->edges(t3);
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			t3EdgeIterEnd = t3EdgeIter + t3Degree;
			t1EdgeIter = t1Edges;
			while (t1EdgeIter != t1EdgeIterEnd && t3EdgeIter != t3EdgeIterEnd) {
#ifndef TIME
				++m_numOfOperations;
#endif
				if (*t1EdgeIter < *t3EdgeIter) {
					++t1EdgeIter;
				} else if (*t1EdgeIter > *t3EdgeIter) {
					++t3EdgeIter;
				} else {
					if (i < *t1EdgeIter && *t1EdgeIter < t3) {
						++m_numOfTriangles;
					}
					++t1EdgeIter;
					++t3EdgeIter;
				}
			}
		} while (++t1OuterEdgeIter != t1EdgeIterEnd);
	}
}
