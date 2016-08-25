/*
 * T2O1.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "T2O1.h"

#include "../../network/Graph.h"
#include <algorithm>

T2O1::T2O1(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

T2O1::~T2O1() {
}

void T2O1::countTriads(Graph* const g) {
	int i = 1;
	int t2OutDegree, t2Degree, t1, t1OutDegree;
	const int* t2OutEdges;
	const int* t2OutEdgesIter;
	const int* t2OutEdgesEnd;
	const int* t2InEdgesIter;
	const int* t2InEdgesEnd;
	const int* t1OutEdgesIter;
	const int* t1OutEdgesEnd;
	const int numOfNodes = g->n() - 1;

	for (; i < numOfNodes; ++i) {
		t2OutDegree = g->outDegree(i);
		t2Degree = g->degree(i);
		// no incoming or outgoing edge
		if (t2OutDegree == 0 || t2OutDegree == t2Degree) {
			continue;
		}
		t2OutEdges = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t2OutEdgesEnd = t2OutEdges + t2OutDegree;
		t2InEdgesIter = t2OutEdgesEnd;
		t2InEdgesEnd = t2OutEdges + t2Degree;
		do {
			t1 = *t2InEdgesIter;
			t1OutDegree = g->outDegree(t1);
			// for a triangle at least 2 edges are needed
			if (t1OutDegree < 2) {
				continue;
			}
			t1OutEdgesIter = g->edges(t1);
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			t1OutEdgesEnd = t1OutEdgesIter + t1OutDegree;
			t2OutEdgesIter = t2OutEdges;
			do {
				t1OutEdgesIter = lower_bound(t1OutEdgesIter, t1OutEdgesEnd,
						*t2OutEdgesIter);
				if (t1OutEdgesIter == t1OutEdgesEnd) {
					break;
				}
				if (*t1OutEdgesIter == *t2OutEdgesIter) {
					++t1OutEdgesIter;
					++m_numOfTriangles;
				}
			} while (++t2OutEdgesIter != t2OutEdgesEnd);
		} while (++t2InEdgesIter != t2InEdgesEnd);
	}
}
