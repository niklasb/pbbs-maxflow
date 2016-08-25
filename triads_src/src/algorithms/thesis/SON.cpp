/*
 * SON.cpp
 *
 *  Created on: 28.05.2013
 *      Author: ortmann
 */

#include "SON.h"
#include "../../network/Graph.h"

SON::SON(string name, OrderingFactory::SORTING sorting) :
		AbstractOrientedTriadAlgorithm(name, sorting) {
}

SON::~SON() {
}

void SON::countTriads(Graph* const g) {
	int numOfNodes = g->n();
	int* const mark = new int[numOfNodes]();
	int i, j, k, t2, t1LastOutInd, t2lastOutInd;
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
			for (j = 0; j < t1LastOutInd; ++j) {
				t2 = t1OutEdges[j];
				t2lastOutInd = g->outDegree(t2) - 1;
				if (t2lastOutInd >= 0) {
					t2OutEdges = g->edges(t2);
#ifndef TIME
					++m_numOfAdjRequest;
#endif
					for (k = 0; k <= t2lastOutInd; ++k) {
#ifndef TIME
						++m_numOfOperations;
#endif
						m_numOfTriangles += mark[t2OutEdges[k]];
					}
				}
				mark[t2] = 0;
			}
			mark[t1OutEdges[t1LastOutInd]] = 0;
		}
	}
	delete[] mark;
}
