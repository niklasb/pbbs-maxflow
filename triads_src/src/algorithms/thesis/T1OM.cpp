/*
 * T1OM.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "T1OM.h"
#include "../../network/Graph.h"
T1OM::T1OM(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting), m_hGraph(0) {
}

T1OM::~T1OM() {
}

void T1OM::prepare(Graph* const g) {
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

void T1OM::countTriads(Graph* const g) {
	int i = 0;
	int t1OutDegree, t2, t2MaxVal;
	const int* t1OuterIter;
	const int* t1InnerIterEnd;
	const int* t1OuterIterEnd;
	const int* t1InnerIter;
	hashSet::const_iterator setEnd;
	const int numOfNodes = g->n() - 2;

	for (; i < numOfNodes; ++i) {
		t1OutDegree = g->outDegree(i);
		if (t1OutDegree < 2) {
			continue;
		}
		t1OuterIter = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t1InnerIterEnd = t1OuterIter + t1OutDegree;
		t1OuterIterEnd = t1InnerIterEnd - 1;
		do {
			t2 = *t1OuterIter;
			hashSet& hSet = m_hGraph[t2];
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			t1InnerIter = ++t1OuterIter;
			if (hSet.size() == 0) {
				continue;
			}
			t2MaxVal = g->edges(t2)[hSet.size() - 1];
			setEnd = hSet.end();
			do {
#ifndef TIME
				++m_numOfOperations;
#endif
				if (*t1InnerIter > t2MaxVal) {
					break;
				}
				if (hSet.find(*t1InnerIter) != setEnd) {
					++m_numOfTriangles;
				}
			} while (++t1InnerIter != t1InnerIterEnd);
		} while (t1OuterIter != t1OuterIterEnd);
	}
	delete[] m_hGraph;
}
