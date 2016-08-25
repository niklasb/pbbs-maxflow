/*
 * IOGraph.h
 *
 *  Created on: 19.05.2013
 *      Author: ortmann
 */

#ifndef IOGRAPH_H_
#define IOGRAPH_H_

#include <vector>
#include <algorithm>

using namespace std;

class IOGraph {
public:
	IOGraph(int numOfNodes);

	~IOGraph();

	int n() const {
		return m_numOfNodes;
	}

	unsigned long m() const {
		return m_numOfEdges;
	}

	bool addEdge(int from, int to) {
		return addEdge(from, to, true);
	}

	vector<int>& edges(int pos) const {
		return m_adjList[pos];
	}

	bool addEdge(int from, int to, bool allowMultEdges) {
		if(from == to){
			return false;
		}
		if (allowMultEdges || !hasEdge(from, to)) {
			m_adjList[from].push_back(to);
			m_adjList[to].push_back(from);
			++m_numOfEdges;
			return true;
		}
		return false;
	}

	unsigned int degree(int index) const {
		return m_adjList[index].size();
	}

	bool hasEdge(int from, int to) const {
		vector<int>& vec = m_adjList[from];
		return find(vec.begin(), vec.end(), to) != vec.end();
	}

	bool removeEdge(int from, int to) {
		if (removeValueFromVector(m_adjList[from], to)
				&& removeValueFromVector(m_adjList[to], from)) {
			--m_numOfEdges;
			return true;
		}
		return false;
	}

	void permute(const int* const permutation);

private:

	bool removeValueFromVector(vector<int>& vec, int valueToErase);

	int m_numOfNodes;
	unsigned long m_numOfEdges;
	vector<int>* m_adjList;

	// disable copy and copy-assignment
	IOGraph(const IOGraph& toCopy);
	IOGraph& operator=(const IOGraph& rhs);

};

#endif /* IOGRAPH_H_ */
