/*
 * DegreeStatistic.cpp
 *
 *  Created on: 16.09.2013
 *      Author: ortmann
 */

#include "DegreeStatistic.h"

#include <math.h>

#include "../../network/Graph.h"

DegreeStatistic::DegreeStatistic(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

DegreeStatistic::~DegreeStatistic() {
}

void DegreeStatistic::countTriads(Graph* const g) {
	int outDeg, inDeg;
	int maxOut = 0;
	for (int i = 0; i < g->n(); ++i) {
		outDeg = g->outDegree(i);
		inDeg = g->degree(i) - outDeg;
		// number of triangle is d^+(v) d^+(v)-1 / 2
		m_numOfTriangles += (outDeg * (outDeg - 1)) / 2;
#ifndef TIME
		// operation count is d^+(v) * d^-(v)
		m_numOfOperations += outDeg * inDeg;
		maxOut = std::max(maxOut, outDeg);
#endif
	}
	// number of adja requestion is max outdegree
	m_numOfAdjRequest = maxOut;
}
