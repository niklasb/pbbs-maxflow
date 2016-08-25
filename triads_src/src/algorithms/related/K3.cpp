/*
 * K3.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "K3.h"
#include "../../network/DLGraph.h"

K3::K3(string name) :
		AbstractTriadAlgorithm(name), m_dlGraph(0) {
}

K3::~K3() {
}

void K3::prepare(Graph* const g) {
	m_dlGraph = new DLGraph(g, OrderingFactory::DESC);
}

void K3::countTriads(Graph* const g) {
	const int numOfNodes = m_dlGraph->n();
	// init with 0's
	int* mark = new int[numOfNodes]();
	DLGraph::Node* t1;
	DLGraph::Node* t2;
	for (int i = 0; i < numOfNodes; ++i) {
		t1 = m_dlGraph->getNode(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		if (!t1) {
			continue;
		}
		t2 = t1;
		while (t1) {
#ifndef TIME
			++m_numOfOperations;
#endif
			mark[t1->getValue()] = 1;
			t1 = t1->next();
		}
		t1 = t2;
		while (t1) {
			t2 = m_dlGraph->getNode(t1->getValue());
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			while (t2) {
#ifndef TIME
				++m_numOfOperations;
#endif
				m_numOfTriangles += mark[t2->getValue()];
				t2 = t2->next();
			}
			mark[t1->getValue()] = 0;
			t1 = t1->next();
		}
		m_dlGraph->removeNode(i);
	}
	delete[] mark;
	delete m_dlGraph;
}
