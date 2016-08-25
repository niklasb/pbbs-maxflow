/*
 * DLGraph.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef DLGRAPH_H_
#define DLGRAPH_H_

#include "../orderings/OrderingFactory.h"

class Graph;

class DLGraph {
public:
	DLGraph(const Graph* const g, OrderingFactory::SORTING sorting);
	~DLGraph();
	inline int n() {
		return m_numberOfNodes;
	}
	class Node {
	public:
		inline Node* next() {
			return m_bot;
		}
		inline int getValue() {
			return m_val;
		}
		Node(int val) :
				m_val(val), m_top(0), m_bot(0), m_doubleLink(0) {
		}

		Node(int val, Node* doubleLink) :
				m_val(val), m_top(0), m_bot(0), m_doubleLink(doubleLink) {
			m_doubleLink->m_doubleLink = this;
		}

		~Node() {
		}
		int m_val;
		Node* m_top;
		Node* m_bot;
		Node* m_doubleLink;
	private:
		Node(const Node& rhs);
		Node& operator=(const Node& rhs);
	};

	void removeDoubleLink(Node* node, int index);

	inline Node* getNode(int index) const {
		return m_graph[index];
	}
	void removeNode(int index);

private:
	DLGraph(const DLGraph& rhs);
	DLGraph& operator=(const DLGraph& rhs);
	Node** m_graph;
	int m_numberOfNodes;
};

#endif /* DLGRAPH_H_ */
