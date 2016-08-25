/*
 * LO1.h
 *
 *  Created on: 29.05.2013
 *      Author: ortmann
 */

#ifndef LO1_H_
#define LO1_H_

#include "AbstractOrientedTriadAlgorithm.h"

class LO1: public AbstractOrientedTriadAlgorithm {
public:
	LO1(string name, OrderingFactory::SORTING sorting);
	~LO1();
protected:
	void countTriads(Graph* const g);
};

#endif /* LO1_H_ */
