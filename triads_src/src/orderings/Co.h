/*
 * Co.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef CO_H_
#define CO_H_

#include "AbstractOrdering.h"

class Co: public AbstractOrdering {
public:
	Co();
	~Co();
	int* getPI(const Graph* const g) const;
	void map(Graph* const g) const;
private:
	Co(const Co&);
	Co& operator=(const Co&);
};

#endif /* CO_H_ */
