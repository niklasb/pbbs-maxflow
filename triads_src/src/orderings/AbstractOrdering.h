/*
 * AbstractOrdering.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef ABSTRACTORDERING_H_
#define ABSTRACTORDERING_H_

#include "../network/Graph.h"

class AbstractOrdering {

public:
	inline int* getInversePI(const Graph* const g, const int* const pi) const {
		int* piInv = new int[g->n()];
		for (int i = 0; i < g->n(); ++i) {
			piInv[pi[i]] = i;
		}
		return piInv;
	}

	virtual ~AbstractOrdering() {
	}

	virtual int* getPI(const Graph* const g) const = 0;
	virtual void map(Graph* const g) const = 0;

protected:
	AbstractOrdering() {
	}

private:
	AbstractOrdering(const AbstractOrdering&);
	AbstractOrdering& operator=(const AbstractOrdering&);

};

#endif /* ABSTRACTORDERING_H_ */
