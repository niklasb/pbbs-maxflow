/*
 * T2OM.cpp
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#include "T2OM.h"
#include "../../network/Graph.h"

T2OM::T2OM(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting), m_hGraph(0) {
}

T2OM::~T2OM() {
}

void T2OM::countTriads(Graph* const g) {
	int i = 1;
	int t2OutDegree, t2Degree, t1, t1MaxVal;
	const int* t2OutEdgesBegin;
	const int* t2InEdgesIter;
	const int* t2OutEdgesEnd;
	const int* t2InEdgesEnd;
	const int* t2OutEdgesIter;
	hashSet::const_iterator setEnd;
	const int numOfNodes = g->n() - 1;
	for (; i < numOfNodes; ++i) {
		t2OutDegree = g->outDegree(i);
		t2Degree = g->degree(i);
		// no incoming or outgoing edge
		if (t2OutDegree == 0 || t2OutDegree == t2Degree) {
			continue;
		}
		t2OutEdgesBegin = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t2OutEdgesEnd = t2OutEdgesBegin + t2OutDegree;
		t2InEdgesIter = t2OutEdgesEnd;
		t2InEdgesEnd = t2OutEdgesBegin + t2Degree;
		do {
			t1 = *t2InEdgesIter;
			hashSet& hSet = m_hGraph[t1];
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			if (hSet.size() < 2) {
				continue;
			}
			t1MaxVal = g->edges(t1)[hSet.size() - 1];
			setEnd = hSet.end();
			t2OutEdgesIter = t2OutEdgesBegin;
			do {
#ifndef TIME
				++m_numOfOperations;
#endif
				if (*t2OutEdgesIter > t1MaxVal) {
					break;
				}
				if (hSet.find(*t2OutEdgesIter) != setEnd) {
					++m_numOfTriangles;
				}
			} while (++t2OutEdgesIter != t2OutEdgesEnd);
		} while (++t2InEdgesIter != t2InEdgesEnd);
	}
	delete[] m_hGraph;
}

void T2OM::prepare(Graph* const g) {
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
