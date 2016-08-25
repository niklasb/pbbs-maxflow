/*
 * NodeIteratorSFO.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "NodeIteratorSFO.h"
#include "../../network/DLGraph.h"
#include <vector>
NodeIteratorSFO::NodeIteratorSFO(string name) :
		AbstractTriadAlgorithm(name), m_dlGraph(0), m_hGraph(0) {
}

NodeIteratorSFO::~NodeIteratorSFO() {
}

void NodeIteratorSFO::prepare(Graph* const g) {
	m_dlGraph = new DLGraph(g, OrderingFactory::SFO);
	const int numOfNodes = m_dlGraph->n();
	m_hGraph = new hashSet[numOfNodes];
	DLGraph::Node* cur;
	std::vector<int> adj;
	for (int i = 0; i < numOfNodes; ++i) {
		cur = m_dlGraph->getNode(i);
		while (cur) {
			adj.push_back(cur->getValue());
			cur = cur->next();
		}
		m_hGraph[i].insert(adj.begin(), adj.end());
		std::vector<int>().swap(adj);
	}
}

void NodeIteratorSFO::countTriads(Graph* const g) {
	const int numOfNodes = m_dlGraph->n();
	DLGraph::Node* outerLoop;
	DLGraph::Node* innerLoop;
	hashSet::const_iterator hSetEnd;
	for (int i = 0; i < numOfNodes; ++i) {
		outerLoop = m_dlGraph->getNode(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		if (!outerLoop) {
			continue;
		}
		while (outerLoop->next()) {
			innerLoop = outerLoop->next();
			hashSet& hSet = m_hGraph[outerLoop->getValue()];
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			hSetEnd = hSet.end();
			while (innerLoop) {
#ifndef TIME
				++m_numOfOperations;
#endif
				if (hSet.find(innerLoop->getValue()) != hSetEnd) {
					++m_numOfTriangles;
				}
				innerLoop = innerLoop->next();
			}
			outerLoop = outerLoop->next();
		}
		m_dlGraph->removeNode(i);
	}
	delete[] m_hGraph;
	delete m_dlGraph;
}
