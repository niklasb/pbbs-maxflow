/*
 * Forward.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "Forward.h"

#include <vector>

#include "../../orderings/OrderingFactory.h"
#include "../../orderings/AbstractOrdering.h"
#include "../../network/Graph.h"

Forward::Forward(string name) :
		AbstractTriadAlgorithm(name), m_pi(0), m_invPi(0) {
}

Forward::~Forward() {
}

void Forward::prepare(Graph* const g) {
	AbstractOrdering* descOrd = OrderingFactory::getOrdering(
			OrderingFactory::DESC);
	m_pi = descOrd->getPI(g);
	m_invPi = descOrd->getInversePI(g, m_pi);
	delete descOrd;
}

void Forward::countTriads(Graph* const g) {
	int i = 0;
	const int numOfNodes = g->n();
	int t2, t3, t2Degree;
	const int* t2EdgeIter;
	const int* t2EdgeIterEnd;
	vector<int>* inEdges = new vector<int> [numOfNodes];
	for (; i < numOfNodes; ++i) {
		inEdges[i].reserve(g->degree(m_pi[i]));
	}
	vector<int>::const_iterator t2InEdgesIter;
	vector<int>::const_iterator t3InEdgesIter;
	vector<int>::const_iterator t2InEdgesEnd;
	vector<int>::const_iterator t3InEdgesEnd;
	for (i = 0; i < numOfNodes; ++i) {
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
		vector<int>& t2InEdges = inEdges[i];
		t2InEdgesEnd = t2InEdges.end();
		for (; t2EdgeIter != t2EdgeIterEnd; ++t2EdgeIter) {
			if ((t3 = m_invPi[*t2EdgeIter]) > i) {
				vector<int>& t3InEdges = inEdges[t3];
#ifndef TIME
				++m_numOfAdjRequest;
#endif
				if (t3InEdges.size() > 0) {
					t2InEdgesIter = t2InEdges.begin();
					t3InEdgesIter = t3InEdges.begin();
					t3InEdgesEnd = t3InEdges.end();
					while (t2InEdgesIter != t2InEdgesEnd
							&& t3InEdgesIter != t3InEdgesEnd) {
#ifndef TIME
						++m_numOfOperations;
#endif
						if (*t2InEdgesIter < *t3InEdgesIter) {
							++t2InEdgesIter;
						} else if (*t2InEdgesIter > *t3InEdgesIter) {
							++t3InEdgesIter;
						} else {
							++m_numOfTriangles;
							++t2InEdgesIter;
							++t3InEdgesIter;
						}
					}
				}
				t3InEdges.push_back(i);
			}
		}
	}
	delete[] inEdges;
	delete[] m_pi;
	delete[] m_invPi;
}
