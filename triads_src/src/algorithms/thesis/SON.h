/*
 * SON.h
 *
 *  Created on: 28.05.2013
 *      Author: ortmann
 */

#ifndef SON_H_
#define SON_H_

#include "AbstractOrientedTriadAlgorithm.h"

class SON: public AbstractOrientedTriadAlgorithm {
public:
	SON(string name, OrderingFactory::SORTING sorting);
	~SON();
protected:
	void countTriads(Graph* const g);
};

#endif /* SON_H_ */
