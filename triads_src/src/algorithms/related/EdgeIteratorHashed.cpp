/*
 * EdgeIteratorHashed.cpp
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#include "EdgeIteratorHashed.h"
#include "../../network/Graph.h"

EdgeIteratorHashed::EdgeIteratorHashed(string name) :
		AbstractTriadAlgorithm(name), m_hGraph(0) {
}

EdgeIteratorHashed::~EdgeIteratorHashed() {
}

void EdgeIteratorHashed::prepare(Graph* const g) {
	const unsigned int numOfNodes = g->n();
	m_hGraph = new hashSet[numOfNodes];
	const int* adja;
	for (unsigned int i = 0; i < numOfNodes; ++i) {
		adja = g->edges(i);
		m_hGraph[i].insert(adja, adja + g->degree(i));
	}
}

void EdgeIteratorHashed::countTriads(Graph* const g) {
	const int numOfNodes = g->n();
	int t1degree, t3;
	const int* t1EdgeIter;
	const int* t1EdgeIterEnd;
	for (int i = 0; i < numOfNodes; ++i) {
		t1degree = g->degree(i);
		if (t1degree < 2) {
			continue;
		}
		t1EdgeIter = g->edges(i);
#ifndef TIME
		++m_numOfAdjRequest;
#endif
		t1EdgeIterEnd = t1EdgeIter + t1degree;
		hashSet& t1hSet = m_hGraph[i];
		do {
			t3 = *t1EdgeIter;
			hashSet& t3hSet = m_hGraph[t3];
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			if (t3hSet.size() < 2) {
				continue;
			}
			if (t3hSet.size() <= t1hSet.size()) {
				intersect(t3hSet, t1hSet, i, t3);
			} else {
				intersect(t1hSet, t3hSet, i, t3);
			}

		} while (++t1EdgeIter != t1EdgeIterEnd);
	}
	delete[] m_hGraph;
}

void EdgeIteratorHashed::intersect(hashSet& search, hashSet& lookup, int t1,
		int t3) {
	hashSet::const_iterator searchEnd = search.end();
	hashSet::const_iterator lookupEnd = lookup.end();
	for (hashSet::const_iterator iter = search.begin(); iter != searchEnd;
			++iter) {
#ifndef TIME
		++m_numOfOperations;
#endif
		if (t1 < *iter && *iter < t3 && lookup.find(*iter) != lookupEnd) {
			++m_numOfTriangles;
		}
	}
}
