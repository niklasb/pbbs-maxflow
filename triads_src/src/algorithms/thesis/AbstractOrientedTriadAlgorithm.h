/*
 * AbstractOrientedTriadAlgorithm.h
 *
 *  Created on: 22.05.2013
 *      Author: ortmann
 */

#ifndef ABSTRACTORIENTEDTRIADALGORITHM_H_
#define ABSTRACTORIENTEDTRIADALGORITHM_H_

#include "../AbstractTriadAlgorithm.h"
#include "../../orderings/OrderingFactory.h"

// class forwarding
class AbstractOrdering;

class AbstractOrientedTriadAlgorithm: public AbstractTriadAlgorithm {
public:
	virtual ~AbstractOrientedTriadAlgorithm();

protected:
	AbstractOrientedTriadAlgorithm(string name, OrderingFactory::SORTING sorting);
	void prepare(Graph* const g);

private:
	const AbstractOrdering* const m_ordering;
	// disable copy and assignment constructor
	AbstractOrientedTriadAlgorithm(const AbstractOrientedTriadAlgorithm&);
	AbstractOrientedTriadAlgorithm& operator=(
			const AbstractOrientedTriadAlgorithm&);
};

#endif /* ABSTRACTORIENTEDTRIADALGORITHM_H_ */
