/*
 * SONless.h
 *
 *  Created on: 09.07.2013
 *      Author: ortmann
 */

#ifndef SONLESS_H_
#define SONLESS_H_

#include "AbstractOrientedTriadAlgorithm.h"

class SON_less: public AbstractOrientedTriadAlgorithm {
public:
	SON_less(string name, OrderingFactory::SORTING sorting);
	~SON_less();
protected:
	void countTriads(Graph* const g);
private:
	void intersectAll(const int* const mark, const int* const t2OutEdges,
			const int& numOfIter);
	void intersectSubset(const int* const mark, const int* t2OutEdges,
			const int& maxVal);
};

#endif /* SONLESS_H_ */
