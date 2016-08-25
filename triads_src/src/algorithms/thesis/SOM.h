/*
 * SOM.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef SOM_H_
#define SOM_H_

#include "AbstractOrientedTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class SOM: public AbstractOrientedTriadAlgorithm {
public:
	SOM(string name, OrderingFactory::SORTING sorting);
	~SOM();
protected:
	void countTriads(Graph* const g);
	void prepare(Graph* const g);
	hashSet* m_hGraph;
};

#endif /* SOM_H_ */
