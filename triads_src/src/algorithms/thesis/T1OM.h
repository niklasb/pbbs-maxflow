/*
 * T1OM.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef T1OM_H_
#define T1OM_H_

#include "AbstractOrientedTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class T1OM: public AbstractOrientedTriadAlgorithm {
public:
	T1OM(string name, OrderingFactory::SORTING sorting);
	~T1OM();
protected:
	void countTriads(Graph* const g);
	void prepare(Graph* const g);
	hashSet* m_hGraph;
};

#endif /* T1OM_H_ */
