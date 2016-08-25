/*
 * Asc.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef ASC_H_
#define ASC_H_

#include "AbstractOrdering.h"

class Asc: public AbstractOrdering {
public:
	Asc();
	~Asc();
	int* getPI(const Graph* const g) const;
	void map(Graph* const g) const;
private:
	Asc(const Asc&);
	Asc& operator=(const Asc&);
};

#endif /* ASC_H_ */
