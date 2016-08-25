/*
 * DLGraph.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "DLGraph.h"
#include <algorithm>
#include "Graph.h"
#include "../orderings/AbstractOrdering.h"

DLGraph::DLGraph(const Graph* const g, OrderingFactory::SORTING sorting) {
	AbstractOrdering* ord = OrderingFactory::getOrdering(sorting);
	const int* const pi = ord->getPI(g);
	const int* const invPi = ord->getInversePI(g, pi);
	m_numberOfNodes = g->n();
	int mapVal, invMapVal;
	const int* adja;
	const int* adjaEnd;
	Node* n1;
	Node* n2;
	Node* head;
	// init with 0's
	m_graph = new Node*[m_numberOfNodes]();
	for (int i = 0; i < m_numberOfNodes; ++i) {
		mapVal = pi[i];
		adja = g->edges(mapVal);
		adjaEnd = adja + g->degree(mapVal);
		for (; adja != adjaEnd; ++adja) {
			if ((invMapVal = invPi[*adja]) > i) {
				n1 = new Node(invMapVal);
				n2 = new Node(i, n1);
				head = m_graph[i];
				if (head) {
					n1->m_bot = head;
					head->m_top = n1;
				}
				m_graph[i] = n1;
				head = m_graph[invMapVal];
				if (head) {
					n2->m_bot = head;
					head->m_top = n2;
				}
				m_graph[invMapVal] = n2;
			}
		}
	}
	delete ord;
	delete[] pi;
	delete[] invPi;
}

DLGraph::~DLGraph() {
	for (int i = 0; i < m_numberOfNodes; ++i) {
		Node* cur;
		Node* next;
		if (m_graph[i] != 0) {
			cur = m_graph[i];
			while (cur) {
				next = cur->m_bot;
				delete cur;
				cur = next;
			}
		}
	}
	delete[] m_graph;
}

void DLGraph::removeNode(int index) {
	Node* cur;
	Node* next;
	if (m_graph[index] != 0) {
		cur = m_graph[index];
		while (cur) {
			removeDoubleLink(cur->m_doubleLink, cur->m_val);
			next = cur->m_bot;
			delete cur;
			cur = next;
		}
	}
	m_graph[index] = 0;
}

void DLGraph::removeDoubleLink(Node* node, int index) {
	Node* top = node->m_top;
	Node* bot = node->m_bot;
	if (top) {
		top->m_bot = bot;
	}
	if (bot) {
		bot->m_top = top;
	}
	delete node;
	if (!top) {
		m_graph[index] = bot;
	}
}
