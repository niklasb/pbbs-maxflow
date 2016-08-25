/*
 * Bfo.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef BFO_H_
#define BFO_H_

#include "AbstractOrdering.h"

class Bfo: public AbstractOrdering {
public:
	Bfo();
	~Bfo();
	int* getPI(const Graph* const g) const;
	void map(Graph* const g) const;
private:
	Bfo(const Bfo&);
	Bfo& operator=(const Bfo&);
};

#endif /* BFO_H_ */
