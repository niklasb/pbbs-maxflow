/*
 * T2O1.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef T2O1_H_
#define T2O1_H_

#include "AbstractOrientedTriadAlgorithm.h"

class T2O1: public AbstractOrientedTriadAlgorithm {
public:
	T2O1(string name, OrderingFactory::SORTING sorting);
	~T2O1();
protected:
	void countTriads(Graph* const g);
};

#endif /* T2O1_H_ */
