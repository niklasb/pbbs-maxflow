/*
 * LO1.cpp
 *
 *  Created on: 29.05.2013
 *      Author: ortmann
 */

#include "LO1.h"
#include "../../network/Graph.h"

LO1::LO1(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

LO1::~LO1() {
}

void LO1::countTriads(Graph* const g) {
	int i;
	int t3FirstInIndex, t3lastInIndex, t1, maxVal;
	const int* t3OuterIter;
	const int* t3OuterIterEnd;
	const int* t3InnerIter;
	const int* t3InnerIterEnd;
	const int* t1Iter;

	const int numOfNodes = g->n();
	for (i = 2; i < numOfNodes; ++i) {
		t3FirstInIndex = g->outDegree(i);
		t3lastInIndex = g->degree(i);
		if (t3lastInIndex - 1 <= t3FirstInIndex) {
			continue;
		}
		t3OuterIter = g->edges(i);
		t3InnerIterEnd = t3OuterIter + t3lastInIndex;
		t3OuterIterEnd = t3InnerIterEnd - 1;
		t3OuterIter += t3FirstInIndex;
		maxVal = *t3OuterIterEnd;
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		do {
			t1 = *t3OuterIter;
			t1Iter = g->edges(t1);
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			t3InnerIter = ++t3OuterIter;
			// works since t1 has an edge to t3 (i) and maxVal < i
			while (t3InnerIter != t3InnerIterEnd && *t1Iter <= maxVal) {
#ifndef TIME
				++m_numOfOperations;
#endif
				if (*t3InnerIter < *t1Iter) {
					++t3InnerIter;
				} else if (*t3InnerIter > *t1Iter) {
					++t1Iter;
				} else {
					++m_numOfTriangles;
					++t3InnerIter;
					++t1Iter;
				}
			}
		} while (t3OuterIter != t3OuterIterEnd);
	}
}
