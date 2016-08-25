/*
 * ForwardHashed.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "ForwardHashed.h"
#include "../../orderings/OrderingFactory.h"
#include "../../orderings/AbstractOrdering.h"

ForwardHashed::ForwardHashed(string name) :
		AbstractTriadAlgorithm(name), m_pi(0), m_invPi(0) {
}
ForwardHashed::~ForwardHashed() {
}

void ForwardHashed::prepare(Graph* const g) {
	AbstractOrdering* descOrd = OrderingFactory::getOrdering(
			OrderingFactory::DESC);
	m_pi = descOrd->getPI(g);
	m_invPi = descOrd->getInversePI(g, m_pi);
	delete descOrd;
}

void ForwardHashed::countTriads(Graph* const g) {
	int i = 0;
	const int numOfNodes = g->n();
	int t2, t3, t2Degree;
	const int* t2EdgeIter;
	const int* t2EdgeIterEnd;
	hashSet* inEdges = new hashSet[numOfNodes];
	for (; i < numOfNodes; ++i) {
		t2 = m_pi[i];
		t2Degree = g->degree(t2);
		if (t2Degree == 0) {
			continue;
		}
		t2EdgeIter = g->edges(t2);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t2EdgeIterEnd = t2EdgeIter + t2Degree;
		hashSet& t2InEdges = inEdges[i];
		for (; t2EdgeIter != t2EdgeIterEnd; ++t2EdgeIter) {
			if ((t3 = m_invPi[*t2EdgeIter]) > i) {
				hashSet& t3InEdges = inEdges[t3];
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				if (t3InEdges.size() > 0) {
					if (t2InEdges.size() > t3InEdges.size()) {
						intersect(t3InEdges, t2InEdges);
					} else {
						intersect(t2InEdges, t3InEdges);
					}
				}
				t3InEdges.insert(i);
			}
		}
	}
	delete[] inEdges;
	delete[] m_pi;
	delete[] m_invPi;
}

void ForwardHashed::intersect(hashSet& search, hashSet& lookup) {
	hashSet::const_iterator searchEnd = search.end();
	hashSet::const_iterator lookupEnd = lookup.end();
	for (hashSet::const_iterator iter = search.begin(); iter != searchEnd;
			++iter) {
#ifndef TIME
		++m_numOfOperations;
#endif
		if (lookup.find(*iter) != lookupEnd) {
			++m_numOfTriangles;
		}
	}
}
