/*
 * EdgeIterator.h
 *
 *  Created on: 02.06.2013
 *      Author: ortmann
 */

#ifndef EDGEITERATOR_H_
#define EDGEITERATOR_H_

#include "../AbstractTriadAlgorithm.h"

class EdgeIterator: public AbstractTriadAlgorithm {
public:
	EdgeIterator(string name);
	~EdgeIterator();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
};

#endif /* EDGEITERATOR_H_ */
