/*
 * Desc.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef DESC_H_
#define DESC_H_

#include "AbstractOrdering.h"

class Desc: public AbstractOrdering {
public:
	Desc();
	~Desc();
	int* getPI(const Graph* const g) const;
	void map(Graph* const g) const;
private:
	Desc(const Desc&);
	Desc& operator=(const Desc&);
};

#endif /* DESC_H_ */
