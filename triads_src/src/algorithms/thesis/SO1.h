/*
 * SO1.h
 *
 *  Created on: 29.05.2013
 *      Author: ortmann
 */

#ifndef SO1_H_
#define SO1_H_

#include "AbstractOrientedTriadAlgorithm.h"

class SO1: public AbstractOrientedTriadAlgorithm {
public:
	SO1(string name, OrderingFactory::SORTING sorting);
	~SO1();
protected:
	void countTriads(Graph* const g);
	void intersect(const int* iter1, const int* iter1End, const int* inter2,
			const int iter2EndVal);
};

#endif /* SO1_H_ */
