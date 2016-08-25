/*
 * T1O1.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef T1O1_H_
#define T1O1_H_

#include "AbstractOrientedTriadAlgorithm.h"

class T1O1: public AbstractOrientedTriadAlgorithm {
public:
	T1O1(string name, OrderingFactory::SORTING sorting);
	~T1O1();
protected:
	void countTriads(Graph* const g);
};

#endif /* T1O1_H_ */
