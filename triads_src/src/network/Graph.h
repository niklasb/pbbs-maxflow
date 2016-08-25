/*
 * Graph.h
 *
 *  Created on: 19.05.2013
 *      Author: ortmann
 */

#ifndef GRAPH_H_
#define GRAPH_H_

// class forwarding
class IOGraph;

class Graph {
public:
	Graph(const int numOfNodes);
	Graph(const int numOfNodes, const int* const degSeq);
	Graph(IOGraph* const g);
	~Graph();

	inline int n() const {
		return m_numOfNodes;
	}

	inline unsigned long m() const {
		return m_numOfEdges;
	}

	bool initAdjList(const int index, const int size);

	bool isInitialized();

	/** Return the edges**/
	inline const int* const edges(const int index) const {
		return m_adjList[index];
	}

	inline int* const modifiableEdges(const int index) const {
		return m_adjList[index];
	}

	bool addEdge(const int from, const int to);

	inline void addOutEdge(const int from, const int to) {
		m_adjList[from][m_outDegree[from]++] = to;
	}

	/** Node i's degree. This is 1 + last index of i's adjacency list.*/
	inline unsigned int degree(const int index) const {
		return m_listSize[index];
	}

	/** Node i's outdegree*/
	inline unsigned int outDegree(const int index) const {
		return m_outDegree[index];
	}

	unsigned int maxDegree() const {
		return m_maxDegree;
	}

	/** updates the ordering*/
	void setOrdering(const int* const pi);
	void setOrdering(const int* const pi, const int* const piInverse);
	/** write the incoming edges*/
	void addIncomingEdges();

	Graph* clone() const;

private:
	Graph(const Graph& rhs);
	Graph& operator=(const Graph& rhs);
	void init();
	const int m_numOfNodes;
	int** m_adjList;
	unsigned int* m_listSize;
	unsigned int* m_outDegree;
	int m_maxDegree;
	unsigned long m_numOfEdges;

};

#endif /* GRAPH_H_ */
