/*
 * SOM.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "SOM.h"
#include "../../network/Graph.h"

SOM::SOM(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting), m_hGraph(0) {
}

SOM::~SOM() {
}

void SOM::countTriads(Graph* const g) {
	int i = 0;
	int t1OutDeg, j, t2, remDeg, t2OutDegree, k;
	const int* t1OutEdges;
	const int* t2OutEdges;
	const int numOfNodes = g->n() - 2;
	hashSet::const_iterator t1End;
	hashSet::const_iterator t2End;

	for (; i < numOfNodes; ++i) {
		t1OutDeg = g->outDegree(i) - 1;
		if (t1OutDeg == 0) {
			continue;
		}
		t1OutEdges = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		hashSet& t1hSet = m_hGraph[i];
		t1End = t1hSet.end();
		remDeg = t1OutDeg;
		for (j = 0; j < t1OutDeg; ++j) {
			t2 = t1OutEdges[j];
			t2OutDegree = g->outDegree(t2);
			if (t2OutDegree > 0) {
				if (remDeg-- <= t2OutDegree) {
					hashSet& t2hSet = m_hGraph[t2];
#ifndef TIME
		++m_numOfAdjRequest;
#endif
					t2End = t2hSet.end();
					for (k = j + 1; k <= t1OutDeg; ++k) {
						if (t2hSet.find(t1OutEdges[k]) != t2End) {
							++m_numOfTriangles;
						}
#ifndef TIME
						++m_numOfOperations;
#endif
					}
				} else {
					t2OutEdges = g->edges(t2);
#ifndef TIME
					++m_numOfAdjRequest;
#endif
					for (k = 0; k < t2OutDegree; ++k) {
						if (t1hSet.find(t2OutEdges[k]) != t1End) {
							++m_numOfTriangles;
						}
#ifndef TIME
						++m_numOfAdjRequest;
#endif
					}
				}
			}
		}
	}
	delete[] m_hGraph;
}

void SOM::prepare(Graph* const g) {
	AbstractOrientedTriadAlgorithm::prepare(g);
	const unsigned int numOfNodes = g->n();
	m_hGraph = new hashSet[numOfNodes];
	const int* iterBegin;
	for (unsigned int i = 0; i < numOfNodes; ++i) {
		iterBegin = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		m_hGraph[i].insert(iterBegin, iterBegin + g->outDegree(i));
	}
}
