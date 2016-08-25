/*
 * AbstractOrientedTriadAlgorithm.cpp
 *
 *  Created on: 22.05.2013
 *      Author: ortmann
 */

#include "AbstractOrientedTriadAlgorithm.h"
#include "../../orderings/AbstractOrdering.h"

//#define MAP

AbstractOrientedTriadAlgorithm::AbstractOrientedTriadAlgorithm(string name,
		OrderingFactory::SORTING sorting) :
		AbstractTriadAlgorithm(name), //
		m_ordering(OrderingFactory::getOrdering(sorting)) {
}

AbstractOrientedTriadAlgorithm::~AbstractOrientedTriadAlgorithm() {
	delete m_ordering;
}

void AbstractOrientedTriadAlgorithm::prepare(Graph* const g) {
#ifdef MAP
	m_ordering->map(g);
#else
	const int* const pi = m_ordering->getPI(g);
	const int* const piReverse = m_ordering->getInversePI(g, pi);
	g->setOrdering(pi, piReverse);
	delete[] piReverse;
	delete[] pi;
#endif
}
