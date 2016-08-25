/*
 * TorusGNP.cpp
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */
#include <math.h>

#include "TorusGNP.h"
#include "../network/IOGraph.h"
#include <iostream>

TorusGNP::TorusGNP() :
		AbstractGenerator(), //
		m_radius(0), //
		m_prob(0) {
}

TorusGNP::~TorusGNP() {
}

void TorusGNP::setRadius(int radius) {
	m_radius = radius;
}

void TorusGNP::setProb(double prob) {
	m_prob = prob;
}

IOGraph* TorusGNP::generate(int numOfNodes) {
	// Create the graph object
	IOGraph* g = new IOGraph(numOfNodes);

	// Create the torus
	for (int i = 0; i < numOfNodes; ++i) {
		for (int j = i + 1; j <= i + m_radius; ++j) {
			g->addEdge(i, j % numOfNodes);
		}
	}
	int overFlow = numOfNodes - m_radius;
	// Add some GNP noise
	long long v = 0;
	long long w = -1;
	double rand;
	double r = 1.0 / log(1.0 - m_prob);
	while (v < numOfNodes) {
		do {
			rand = randDouble();
		} while (!rand);
		w = w + 1 + (long long) (r * log(rand));
		while (w >= v && v < numOfNodes) {
			w = w - v;
			++v;
		}
		if (v < numOfNodes) {
			if (v - w <= m_radius || v - w >= overFlow) {
				if (!g->removeEdge(v, w)) {
					std::cout << "cannot remove edge (" << v << "," << w << ");"
							<< std::endl;
					throw "Cannot remove edge";
				}
			} else {
				if (!g->addEdge(v, w, false)) {
					std::cout << "cannot add edge (" << v << "," << w << ");"
							<< std::endl;
					throw "well rlly bad";
				}
			}
		}
	}
	// return the graph
	return g;
}
