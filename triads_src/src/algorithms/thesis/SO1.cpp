/*
 * SO1.cpp
 *
 *  Created on: 29.05.2013
 *      Author: ortmann
 */

#include "SO1.h"

#include "../../network/Graph.h"

SO1::SO1(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

SO1::~SO1() {
}

void SO1::countTriads(Graph* const g) {
	int i, maxValT1, t2, minMaxVal;
	unsigned t1OutDegree, t2OutDegree;
	const int* t1OutEdgesStart;
	const int* t1OutEdgesOuterIter;
	const int* t1OutEdgesOuterEnd;
	const int* t1OutEdgesInnerEnd;
	const int* t2OutEdges;
	const int numOfNodes = g->n() - 2;
	for (i = 0; i < numOfNodes; ++i) {
		t1OutDegree = g->outDegree(i);
		if (t1OutDegree < 2) {
			continue;
		}
		t1OutEdgesStart = g->edges(i);
		t1OutEdgesOuterIter = t1OutEdgesStart;
		t1OutEdgesInnerEnd = t1OutEdgesStart + t1OutDegree;
		t1OutEdgesOuterEnd = t1OutEdgesInnerEnd - 1;
		maxValT1 = *t1OutEdgesOuterEnd;
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		do {
			t2 = *t1OutEdgesOuterIter;
			++t1OutEdgesOuterIter;
			t2OutDegree = g->outDegree(t2);
			if (t2OutDegree > 0) {

				t2OutEdges = g->edges(t2);
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				// set the iterators accordingly
				if (maxValT1 <= (minMaxVal = t2OutEdges[t2OutDegree - 1])) {
					intersect(t1OutEdgesOuterIter, t1OutEdgesInnerEnd,
							t2OutEdges, maxValT1);
				} else {
					intersect(t2OutEdges, t2OutEdges + t2OutDegree,
							t1OutEdgesOuterIter, minMaxVal);
				}
			}
		} while (t1OutEdgesOuterIter != t1OutEdgesOuterEnd);
	}
}

void SO1::intersect(const int* iter1, const int* iter1End, const int* iter2,
		const int iter2EndVal) {
	// now intersect
	while (iter1 != iter1End && *iter2 <= iter2EndVal) {
#ifndef TIME
		++m_numOfOperations;
#endif
		if (*iter1 < *iter2) {
			++iter1;
		} else if (*iter1 > *iter2) {
			++iter2;
		} else {
			++m_numOfTriangles;
			++iter1;
			++iter2;
		}
	}
}
