/*
 * T1O1.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "T1O1.h"
#include <algorithm>
#include "../../network/Graph.h"

T1O1::T1O1(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

T1O1::~T1O1() {
}

void T1O1::countTriads(Graph* const g) {
	int i = 0;
	int t1OutDeg, t2, t2OutDeg;
	const int* t1OuterIter;
	const int* t1OuterIterEnd;
	const int* t1InnerIter;
	const int* t1InnerIterEnd;
	const int* t2OutIter;
	const int* t2OutIterEnd;
	const int numOfNodes = g->n() - 2;

	for (; i < numOfNodes; ++i) {
		t1OutDeg = g->outDegree(i);
		if (t1OutDeg < 2) {
			continue;
		}
		t1OuterIter = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t1InnerIterEnd = t1OuterIter + t1OutDeg;
		t1OuterIterEnd = t1InnerIterEnd - 1;
		do {
			t2 = *t1OuterIter;
			t1InnerIter = ++t1OuterIter;
			t2OutDeg = g->outDegree(t2);
			if (t2OutDeg == 0) {
				continue;
			}
			t2OutIter = g->edges(t2);
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			t2OutIterEnd = t2OutIter + t2OutDeg;
			do {
				t2OutIter = lower_bound(t2OutIter, t2OutIterEnd, *t1InnerIter);
				if (t2OutIter == t2OutIterEnd) {
					break;
				}
				if (*t2OutIter == *t1InnerIter) {
					++t2OutIter;
					++m_numOfTriangles;
				}
			} while (++t1InnerIter != t1InnerIterEnd);
		} while (t1OuterIter != t1OuterIterEnd);
	}
}
