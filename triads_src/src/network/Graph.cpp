/*
 * Graph.cpp
 *
 *  Created on: 19.05.2013
 *      Author: ortmann
 */

#include "Graph.h"
#include "IOGraph.h"

Graph::Graph(const int numOfNodes) :
		m_numOfNodes(numOfNodes) {
	init();
}

Graph::Graph(const int numOfNodes, const int* const degSeq) :
		m_numOfNodes(numOfNodes) {
	init();
	for (int i = 0; i < numOfNodes; ++i) {
		initAdjList(i, degSeq[i]);
	}
}

Graph::Graph(IOGraph* const g) :
		m_numOfNodes(g->n()) {
	init();
	for (int i = 0; i < m_numOfNodes; ++i) {
		initAdjList(i, g->degree(i));
		std::vector<int>& adj = g->edges(i);
		for (std::vector<int>::const_iterator iter = adj.begin();
				iter != adj.end(); ++iter) {
			m_adjList[i][--m_outDegree[i]] = *iter;
		}
		vector<int>().swap(adj);
	}
	m_numOfEdges = 2 * g->m();
}

Graph::~Graph() {
	for (int i = 0; i < m_numOfNodes; ++i) {
		delete[] m_adjList[i];
	}
	delete[] m_adjList;
	delete[] m_outDegree;
	delete[] m_listSize;
}

bool Graph::initAdjList(const int index, const int size) {
	// check whether there is already an adjacency list initialized
	if (m_adjList[index] != 0) {
		return false;
	}
	// initialize the adjacency list at the given index
	m_adjList[index] = new int[size];
	m_listSize[index] = size;
	m_outDegree[index] = size;
	// update the max degree
	if (m_maxDegree < size) {
		m_maxDegree = size;
	}
	m_numOfEdges += size;
	return true;
}

bool Graph::isInitialized() {
	for (int i = 0; i < m_numOfNodes; ++i) {
		if (m_adjList[i] == 0 || m_outDegree[i] != 0) {
			return false;
		}
	}
	m_numOfEdges /= 2;
	return true;
}

bool Graph::addEdge(const int from, const int to) {
	// not initialized
	if (m_adjList[from] == 0 || m_adjList[to] == 0) {
		return false;
	}
	// more adds then storage
	if (m_outDegree[from] == 0 || m_outDegree[to] == 0) {
		return false;
	}
	m_adjList[from][--m_outDegree[from]] = to;
	m_adjList[to][--m_outDegree[to]] = from;
	return true;
}

void Graph::setOrdering(const int* const pi) {
	int** const tmpAdjList = new int*[m_numOfNodes];
	unsigned int* const tmpListSize = new unsigned int[m_numOfNodes];
	unsigned int* const tmpOutDegree = new unsigned int[m_numOfNodes];
	// permute the nodes according pi
	for (int i = 0; i < m_numOfNodes; ++i) {
		const int val = pi[i];
		tmpListSize[i] = m_listSize[val];
		tmpAdjList[i] = m_adjList[val];
		tmpOutDegree[i] = m_outDegree[val];
	}
	// clear the storage
	delete[] m_listSize;
	delete[] m_adjList;
	delete[] m_outDegree;
	// update the references
	m_adjList = tmpAdjList;
	m_listSize = tmpListSize;
	m_outDegree = tmpOutDegree;
}

void Graph::setOrdering(const int* const pi, const int* const piInverse) {
	int** const __restrict tmpAdjList = new int*[m_numOfNodes];
	unsigned int* const __restrict tmpListSize = new unsigned int[m_numOfNodes];
	// permute the nodes according pi
	for (int i = 0; i < m_numOfNodes; ++i) {
		int size = m_listSize[pi[i]];
		tmpListSize[i] = size;
		int* const __restrict neigh = m_adjList[pi[i]];
		tmpAdjList[i] = neigh;
		unsigned int& outDegI = m_outDegree[i];
		outDegI = 0;
		// update the nodes indices
		int j = 0;
		while (j < size) {
			const int val = piInverse[neigh[j]];
			if (val > i) {
				neigh[j] = val;
				++outDegI;
				++j;
			} else {
				--size;
				neigh[j] = neigh[size];
				neigh[size] = val;
			}
		}
		// sort the outedges ascending
		sort(neigh, neigh + j);
		// sort the inedges descending
		sort(neigh + j, neigh + tmpListSize[i]);
	}
	// clear the storage
	delete[] m_listSize;
	delete[] m_adjList;
	// update the references
	m_adjList = tmpAdjList;
	m_listSize = tmpListSize;
}

void Graph::addIncomingEdges() {
	int i, w;
	unsigned int j, neighSize;
	int* neighbors;
	unsigned int* __restrict outDegCopy = new unsigned int[m_numOfNodes];
	for (i = 0; i < m_numOfNodes; ++i) {
		outDegCopy[i] = m_outDegree[i];
	}
	for (i = 0; i < m_numOfNodes; ++i) {
		// iterate over the out edges and add them to the inedges of the corresponding target
		neighSize = m_outDegree[i];
		neighbors = m_adjList[i];
		for (j = 0; j < neighSize; ++j) {
			w = neighbors[j];
			m_adjList[w][outDegCopy[w]++] = i;
		}
	}
	delete[] outDegCopy;
}

Graph* Graph::clone() const {
	return new Graph(*this);
}

Graph::Graph(const Graph& rhs) :
		m_numOfNodes(rhs.m_numOfNodes), //
		m_adjList(new int*[m_numOfNodes]), //
		m_listSize(new unsigned int[m_numOfNodes]), //
		m_outDegree(new unsigned int[m_numOfNodes]()), //
		m_maxDegree(0), //
		m_numOfEdges(rhs.m_numOfEdges) {
	for (int i = 0; i < m_numOfNodes; ++i) {
		int size = rhs.m_listSize[i];
		m_listSize[i] = size;
		int* adja = new int[size];
		const int* const adjaRhs = rhs.m_adjList[i];
		if (m_maxDegree < size) {
			m_maxDegree = size;
		}
		for (int j = 0; j < size; ++j) {
			adja[j] = adjaRhs[j];
		}
		m_adjList[i] = adja;
	}
}

void Graph::init() {
	m_adjList = new int*[m_numOfNodes];
	m_listSize = new unsigned int[m_numOfNodes];
	m_outDegree = new unsigned int[m_numOfNodes];
	m_maxDegree = 0;
	// init with 0's and 1's
	for (int i = 0; i < m_numOfNodes; ++i) {
		m_adjList[i] = 0;
		m_outDegree[i] = 1;
	}
	m_numOfEdges = 0;
}
