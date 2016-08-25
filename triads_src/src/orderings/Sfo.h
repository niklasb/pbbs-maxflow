/*
 * Sfo.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef SFO_H_
#define SFO_H_

#include "AbstractOrdering.h"

class Sfo: public AbstractOrdering {
public:
	Sfo();
	~Sfo();
	int* getPI(const Graph* const g) const;
	void map(Graph* const g) const;
private:
	Sfo(const Sfo&);
	Sfo& operator=(const Sfo&);
};

#endif /* SFO_H_ */
