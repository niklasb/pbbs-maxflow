/*
 * LOM.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "LOM.h"
#include "../../network/Graph.h"

LOM::LOM(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting), m_hInGraph(0), m_hOutGraph(
				0) {
}

LOM::~LOM() {
}

void LOM::countTriads(Graph* const g) {
	int i = 0;
	int t3FirstInIndex, t3LastInIndex, j, t1, t1OutDegree, t3RemInEdges, k;
	const int* t3InEdges;
	const int* t1OutEdges;
	const int numOfNodes = g->n();
	int* const outIndices = new int[numOfNodes];
	hashSet::const_iterator t3End;
	hashSet::const_iterator t1End;
	for (i = 0; i < numOfNodes; ++i) {
		outIndices[i] = g->outDegree(i);
	}
	for (--i; i > 1; --i) {
		t3FirstInIndex = g->outDegree(i);
		t3LastInIndex = g->degree(i) - 1;
		if ((t3RemInEdges = t3LastInIndex - t3FirstInIndex) <= 0) {
			continue;
		}
		t3InEdges = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		hashSet& t3hSet = m_hInGraph[i];
		t3End = t3hSet.end();
		for (j = t3FirstInIndex; j < t3LastInIndex; ++j) {
			t1 = t3InEdges[j];
			t1OutDegree = --outIndices[t1];
			if (t3RemInEdges <= t1OutDegree) {
				hashSet& t1hSet = m_hOutGraph[t1];
#ifndef TIME
		++m_numOfAdjRequest;
#endif
				t1End = t1hSet.end();
				for (k = j + 1; k <= t3LastInIndex; ++k) {
					if (t1hSet.find(t3InEdges[k]) != t1End) {
						++m_numOfTriangles;
					}
#ifndef TIME
					++m_numOfOperations;
#endif
				}
			} else {
				t1OutEdges = g->edges(t1);
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				for (k = 0; k < t1OutDegree; ++k) {
					if (t3hSet.find(t1OutEdges[k]) != t3End) {
						++m_numOfTriangles;
					}
				}
#ifndef TIME
				m_numOfOperations += t1OutDegree;
#endif
			}
			--t3RemInEdges;
		}
		--outIndices[t3InEdges[t3LastInIndex]];
	}
	delete[] outIndices;
	delete[] m_hOutGraph;
	delete[] m_hInGraph;

}
void LOM::prepare(Graph* const g) {
	AbstractOrientedTriadAlgorithm::prepare(g);
	const unsigned int numOfNodes = g->n();
	m_hInGraph = new hashSet[numOfNodes];
	m_hOutGraph = new hashSet[numOfNodes];
	const int* iterBegin;
	const int* iterEnd;
	for (unsigned int i = 0; i < numOfNodes; ++i) {
		iterBegin = g->edges(i);
		iterEnd = iterBegin + g->outDegree(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		m_hOutGraph[i].insert(iterBegin, iterEnd);
		m_hInGraph[i].insert(iterEnd, iterBegin + g->degree(i));
		if (m_hOutGraph[i].size() != g->outDegree(i)
				|| m_hInGraph[i].size() != g->degree(i) - g->outDegree(i)) {
			throw "fuck you";
		}
	}
}
