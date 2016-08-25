/*
 * LON.cpp
 *
 *  Created on: 28.05.2013
 *      Author: ortmann
 */

#include "LON.h"
#include "../../network/Graph.h"
#include <iostream>
using namespace std;

LON::LON(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

LON::~LON() {
}

void LON::countTriads(Graph* const g) {
	int t3LastInIndex, t3FirstInIndex, t1, t1OutDeg, k;
	const int* t3InEdges;
	const int* t3InEdgesIter;
	const int* t3InEdgesEnd;
	const int* t1OutEdges;
	unsigned int numOfNodes = g->n();
	int* const outDeg = new int[numOfNodes]();
	int* const mark = new int[numOfNodes]();
	for (unsigned int i = 1; i < numOfNodes; ++i) {
		t3FirstInIndex = g->outDegree(i);
		t3LastInIndex = g->degree(i) - 1;
		if (t3LastInIndex - t3FirstInIndex <= 0) {
			if (t3LastInIndex == t3FirstInIndex) {
				++outDeg[g->edges(i)[t3LastInIndex]];
#ifndef TIME
				++m_numOfAdjRequest;
#endif
			}
			continue;
		}
		t3InEdges = g->edges(i);
		t3InEdgesEnd = t3InEdges + t3LastInIndex;
		t3InEdges += t3FirstInIndex;
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		// no need to mark the incoming neighbor with the lowest value
		for (t3InEdgesIter = t3InEdgesEnd; t3InEdgesIter != t3InEdges;
				--t3InEdgesIter) {
#ifndef TIME
			++m_numOfOperations;
#endif
			mark[*t3InEdgesIter] = 1;
		}
		for (; t3InEdgesIter != t3InEdgesEnd; ++t3InEdgesIter) {
			t1 = *t3InEdgesIter;
			if ((t1OutDeg = outDeg[t1]++) > 0) {
				t1OutEdges = g->edges(t1);
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				// not there is at least one edge with a value > maxVal namely i
				for (k = 0; k < t1OutDeg; ++k) {
#ifndef TIME
					++m_numOfOperations;
#endif
					m_numOfTriangles += mark[t1OutEdges[k]];
				}
			}
			mark[t1] = 0;
		}
		mark[*t3InEdgesEnd] = 0;
		++outDeg[*t3InEdgesEnd];
	}
	delete[] outDeg;
	delete[] mark;
}
