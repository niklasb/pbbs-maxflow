/*
 * LOM.h
 *
 *  Created on: 31.05.2013
 *      Author: ortmann
 */

#ifndef LOM_H_
#define LOM_H_

#include "AbstractOrientedTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class LOM: public AbstractOrientedTriadAlgorithm {
public:
	LOM(string name, OrderingFactory::SORTING sorting);
	~LOM();
protected:
	void countTriads(Graph* const g);
	void prepare(Graph* const g);
	hashSet* m_hInGraph;
	hashSet* m_hOutGraph;
};

#endif /* LOM_H_ */
