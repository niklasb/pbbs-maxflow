/*
 * DegreeStatistic.h
 *
 *  Created on: 16.09.2013
 *      Author: ortmann
 */

#ifndef DEGREESTATISTIC_H_
#define DEGREESTATISTIC_H_

#include "AbstractOrientedTriadAlgorithm.h"

class DegreeStatistic: public AbstractOrientedTriadAlgorithm {
public:
	DegreeStatistic(string name, OrderingFactory::SORTING sorting);
	~DegreeStatistic();
protected:
	void countTriads(Graph* const g);
};

#endif /* DEGREESTATISTIC_H_ */
