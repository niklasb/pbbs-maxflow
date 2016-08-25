/*
 * T2OM.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef T2OM_H_
#define T2OM_H_

#include "AbstractOrientedTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class T2OM: public AbstractOrientedTriadAlgorithm {
public:
	T2OM(string name, OrderingFactory::SORTING sorting);
	~T2OM();
protected:
	void countTriads(Graph* const g);
	void prepare(Graph* const g);
	hashSet* m_hGraph;
};

#endif /* T2OM_H_ */
