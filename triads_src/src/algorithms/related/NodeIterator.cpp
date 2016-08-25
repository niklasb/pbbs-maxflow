/*
 * NodeIterator.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "NodeIterator.h"
#include "../../network/Graph.h"
NodeIterator::NodeIterator(string name) :
		AbstractTriadAlgorithm(name), m_hGraph(0) {
}

NodeIterator::~NodeIterator() {
}

void NodeIterator::prepare(Graph* const g) {
	const unsigned int numOfNodes = g->n();
	m_hGraph = new hashSet[numOfNodes];
	const int* adja;
	for (unsigned int i = 0; i < numOfNodes; ++i) {
		adja = g->edges(i);
		m_hGraph[i].insert(adja, adja + g->degree(i));
	}
}

void NodeIterator::countTriads(Graph* const g) {
	const int numOfNodes = g->n();
	int iDeg, min, max;
	const int* iAdja;
	const int* iAdjaOuterIter;
	const int* iAdjaInnerIter;
	const int* iAdjaOuterEnd;
	hashSet::const_iterator hSetEnd;
	for (int i = 0; i < numOfNodes; ++i) {
		iDeg = g->degree(i);
		if (iDeg < 2) {
			continue;
		}
		iAdja = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		iAdjaOuterIter = iAdja + 1;
		iAdjaOuterEnd = iAdja + iDeg;
		do {
			hashSet& hSet = m_hGraph[*iAdjaOuterIter];
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			hSetEnd = hSet.end();
			if (hSet.size() > 0) {
				iAdjaInnerIter = iAdja;
				do {
#ifndef TIME
					++m_numOfOperations;
#endif
					min = std::min(*iAdjaOuterIter, *iAdjaInnerIter);
					max = std::max(*iAdjaOuterIter, *iAdjaInnerIter);
					if ((unsigned int) (i - min) < (unsigned int) (max - min)
							&& hSet.find(*iAdjaInnerIter) != hSetEnd) {
						++m_numOfTriangles;
					}
				} while (++iAdjaInnerIter != iAdjaOuterIter);
			}
		} while (++iAdjaOuterIter != iAdjaOuterEnd);
	}
	delete[] m_hGraph;
}
