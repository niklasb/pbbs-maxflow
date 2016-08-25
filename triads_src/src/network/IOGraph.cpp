/*
 * IOGraph.cpp
 *
 *  Created on: 19.05.2013
 *      Author: ortmann
 */

#include "IOGraph.h"

using namespace std;

IOGraph::IOGraph(int numOfNodes) :
		m_numOfNodes(numOfNodes), m_numOfEdges(0), m_adjList(
				new vector<int> [numOfNodes]) {
}

IOGraph::~IOGraph() {
	delete[] m_adjList;
}

void IOGraph::permute(const int* const permutation) {
	int* revPerm = new int[m_numOfNodes];
	for (int i = 0; i < m_numOfNodes; ++i) {
		revPerm[permutation[i]] = i;
	}
	vector<int> *tmp = new vector<int> [m_numOfNodes];
	for (int i = 0; i < m_numOfNodes; ++i) {
		vector<int>& adja = m_adjList[permutation[i]];
		for (vector<int>::iterator iter = adja.begin(); iter != adja.end();
				++iter) {
			*iter = revPerm[*iter];
		}
		tmp[i].swap(m_adjList[permutation[i]]);
	}
	delete[] revPerm;
	delete[] m_adjList;
	m_adjList = tmp;
}

bool IOGraph::removeValueFromVector(vector<int>& vec, int valueToErase) {
	for (vector<int>::iterator iter = vec.begin(); iter != vec.end(); ++iter) {
		if (*iter == valueToErase) {
			vec.erase(iter);
			return true;
		}
	}
	return false;
}
