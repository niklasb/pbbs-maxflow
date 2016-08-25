/*
 * CompactForward.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "CompactForward.h"
#include <algorithm>
#include "../../orderings/OrderingFactory.h"
#include "../../orderings/AbstractOrdering.h"
#include "../../network/Graph.h"

CompactForward::CompactForward(string name) :
		AbstractTriadAlgorithm(name) {
}

CompactForward::~CompactForward() {
}

void CompactForward::prepare(Graph* const g) {
	AbstractOrdering* descOrd = OrderingFactory::getOrdering(
			OrderingFactory::DESC);
	int* pi = descOrd->getPI(g);
	int* invPi = descOrd->getInversePI(g, pi);
	g->setOrdering(pi);
	int* adja;
	int* adjaEnd;
	int val;
	const int numOfNodes = g->n();
	for (int i = 0; i < numOfNodes; ++i) {
		adja = g->modifiableEdges(i);
		adjaEnd = adja + g->degree(i);
		while (adja != adjaEnd) {
			val = invPi[*adja];
			if (val > i) {
				--adjaEnd;
				*adja = *adjaEnd;
				*adjaEnd = val;
			} else {
				*adja = val;
				++adja;
			}
		}
		sort(g->modifiableEdges(i), adja);
	}
	delete descOrd;
	delete[] pi;
	delete[] invPi;
}

void CompactForward::countTriads(Graph* const g) {
	const int numOfNodes = g->n();
	int i, t3Degree, t2, t2Degree;
	const int* t3InEdgesOuterIter;
	const int* t3InEdgesStart;
	const int* t3InEdgesInnerIter;
	const int* t3InEdgesEnd;
	const int* t2InEdgesIter;
	for (i = 2; i < numOfNodes; ++i) {
		t3Degree = g->degree(i);
		if (t3Degree == 0) {
			continue;
		}
		t3InEdgesStart = g->edges(i);
		t3InEdgesOuterIter = t3InEdgesStart;
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t3InEdgesEnd = t3InEdgesOuterIter + t3Degree;
		for (; t3InEdgesOuterIter != t3InEdgesEnd; ++t3InEdgesOuterIter) {
			t2 = *t3InEdgesOuterIter;
			// first outgoing edge
			if (t2 > i) {
				break;
			}
			t2Degree = g->degree(t2);
			if (t2Degree > 0) {
				t2InEdgesIter = g->edges(t2);
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				t3InEdgesInnerIter = t3InEdgesStart;
				while (t3InEdgesInnerIter != t3InEdgesOuterIter
						&& *t2InEdgesIter < t2) {
#ifndef TIME
					++m_numOfOperations;
#endif

					if (*t3InEdgesInnerIter < *t2InEdgesIter) {
						++t3InEdgesInnerIter;
					} else if (*t3InEdgesInnerIter > *t2InEdgesIter) {
						++t2InEdgesIter;
					} else {
						++m_numOfTriangles;
						++t3InEdgesInnerIter;
						++t2InEdgesIter;
					}
				}
			}
		}
	}
}
