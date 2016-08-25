/*
 * SONless.cpp
 *
 *  Created on: 09.07.2013
 *      Author: ortmann
 */

#include "SONless.h"
#include "../../network/Graph.h"

SON_less::SON_less(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

SON_less::~SON_less() {
}

void SON_less::countTriads(Graph* const g) {
	int numOfNodes = g->n();
	int* const mark = new int[numOfNodes]();
	int i, j, t2, t1LastOutInd, t2lastOutInd, maxt1;
	const int* t1OutEdges;
	const int* t2OutEdges;
	numOfNodes -= 1;
	for (i = 0; i < numOfNodes; ++i) {
		t1LastOutInd = g->outDegree(i) - 1;
		if (t1LastOutInd > 0) {
			t1OutEdges = g->edges(i);
#ifndef TIME
			++m_numOfAdjRequest;
#endif
			// mark the nodes
			for (j = 1; j <= t1LastOutInd; ++j) {
#ifndef TIME
				++m_numOfOperations;
#endif
				mark[t1OutEdges[j]] = 1;
			}
			maxt1 = t1OutEdges[t1LastOutInd];
			for (j = 0; j < t1LastOutInd; ++j) {
				t2 = t1OutEdges[j];
				t2lastOutInd = g->outDegree(t2) - 1;
				if (t2lastOutInd >= 0) {
					t2OutEdges = g->edges(t2);
#ifndef TIME
					++m_numOfAdjRequest;
#endif
					if (t2OutEdges[t2lastOutInd] <= maxt1) {
						intersectAll(mark, t2OutEdges, t2lastOutInd);
					} else {
						intersectSubset(mark, t2OutEdges, maxt1);
					}
				}
				mark[t2] = 0;
			}
			mark[maxt1] = 0;
		}
	}
	delete[] mark;
}

void SON_less::intersectAll(const int* const mark, const int* const t2OutEdges,
		const int& numOfIter) {
	for (int k = 0; k <= numOfIter; ++k) {
#ifndef TIME
		++m_numOfOperations;
#endif
		m_numOfTriangles += mark[t2OutEdges[k]];
	}
}

void SON_less::intersectSubset(const int* const mark, const int* t2OutEdges,
		const int& maxVal) {
	while (*t2OutEdges <= maxVal) {
#ifndef TIME
		++m_numOfOperations;
#endif
		m_numOfTriangles += mark[*t2OutEdges];
		++t2OutEdges;
	}
}
