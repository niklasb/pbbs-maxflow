/*
 * PrefAttTriaClosure.cpp
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#include "PrefAttTriaClosure.h"
#include "../network/IOGraph.h"

using namespace std;
PrefAttTriaClosure::PrefAttTriaClosure() :
		AbstractGenerator(), //
		m_outDegree(0), //
		m_triadFormationProb(0) {
}

PrefAttTriaClosure::~PrefAttTriaClosure() {
}

void PrefAttTriaClosure::setDegree(int degree) {
	m_outDegree = degree;
}

void PrefAttTriaClosure::setTriadFormProb(double prob) {
	m_triadFormationProb = prob;
}

IOGraph* PrefAttTriaClosure::generate(int numOfNodes) {
	if (numOfNodes <= 0) {
		throw "PREF:: n <= 0";
	}
	if (m_outDegree < 0 || m_outDegree > numOfNodes) {
		throw "PREF:: d < 0 or d > n";
	}
	IOGraph * g = new IOGraph(numOfNodes);
	// 2 * current number of edges
	int m = 0;
	// the index of last node that received an edge from a preferential attachment step
	int lastPAInd;
	// array storing the degrees each node pair
	int * arr = new int[2 * numOfNodes * m_outDegree];
	vector<int> feasibleCandidates;
	// definition of preferential attachment according Bollobás.
	for (int i = 0; i < numOfNodes; i++) {
		// reset the last pa index for each node (no pa step yet)
		lastPAInd = -1;
		// create every outedge
		for (int j = 0; j < m_outDegree; j++) {
			// copy the current node to m
			arr[m] = i;
			// switch PA and TF step
			bool doPAStep = true;
			if (lastPAInd >= 0 && randDouble() < m_triadFormationProb) {
				// triad formation step
				vector<int>& adj = g->edges(lastPAInd);
				// get the feasible candidates, these are those neighbors
				// that are not yet connected with i
				for (int k = 0; k < (int) adj.size(); k++) {
					if (!g->hasEdge(i, adj[k])) {
						feasibleCandidates.push_back(adj[k]);
					}
				}
				if (!feasibleCandidates.empty()) {
					doPAStep = false;
					arr[m + 1] = feasibleCandidates.at(
							(int) (randDouble() * feasibleCandidates.size()));
				}
				vector<int>().swap(feasibleCandidates);
			}
			if (doPAStep) {
				arr[m + 1] = arr[(int) (randDouble() * (m + 1))];
			}
			lastPAInd = arr[m + 1];
			g->addEdge(arr[m], arr[m + 1], false);
			m += 2;
		}
	}
	delete[] arr;
	return g;
}
