/*
 * LON.h
 *
 *  Created on: 28.05.2013
 *      Author: ortmann
 */

#ifndef LON_H_
#define LON_H_

#include "AbstractOrientedTriadAlgorithm.h"

class LON: public AbstractOrientedTriadAlgorithm {
public:
	LON(string name, OrderingFactory::SORTING sorting);
	~LON();
protected:
	void countTriads(Graph* const g);
};

#endif /* LON_H_ */
