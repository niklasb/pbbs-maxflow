/*
 * AbstractTriadAlgorithm.cpp
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#include "AbstractTriadAlgorithm.h"
#include <iostream>
using namespace std;

AbstractTriadAlgorithm::AbstractTriadAlgorithm(string name) :
		m_numOfTriangles(0), //
#ifndef TIME
				m_numOfOperations(0), //
				m_numOfAdjRequest(0),//
#endif
		m_algoName(name) {
}

AbstractTriadAlgorithm::~AbstractTriadAlgorithm() {
}

const string& AbstractTriadAlgorithm::getName() const {
	return m_algoName;
}

timeval AbstractTriadAlgorithm::getRuntimeAlgorithm() {
	return m_timeMeasure.getAlgoTime();
}

timeval AbstractTriadAlgorithm::getRuntimeOrdering() {
	return m_timeMeasure.getOrderingTime();
}

timeval AbstractTriadAlgorithm::getRuntimeTotal() {
	return m_timeMeasure.getTotalTime();
}

unsigned long long AbstractTriadAlgorithm::getTriangleCount() {
	return m_numOfTriangles;
}

#ifndef TIME

unsigned long long AbstractTriadAlgorithm::getOperationCount() {
	return m_numOfOperations;
}

unsigned long long AbstractTriadAlgorithm::getAdjRequestCount() {
	return m_numOfAdjRequest;
}

void AbstractTriadAlgorithm::resetValues() {
	m_numOfOperations = 0;
	m_numOfAdjRequest = 0;
}
#endif

