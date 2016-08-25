/*
 * CompactForward.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef COMPACTFORWARD_H_
#define COMPACTFORWARD_H_

#include "../AbstractTriadAlgorithm.h"

class CompactForward: public AbstractTriadAlgorithm {
public:
	CompactForward(string name);
	~CompactForward();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
};

#endif /* COMPACTFORWARD_H_ */
