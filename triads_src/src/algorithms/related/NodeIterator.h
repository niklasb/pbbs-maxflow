/*
 * NodeIterator.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef NODEITERATOR_H_
#define NODEITERATOR_H_

#include "../AbstractTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;


class NodeIterator: public AbstractTriadAlgorithm {
public:
	NodeIterator(string name);
	~NodeIterator();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	hashSet* m_hGraph;
};

#endif /* NODEITERATOR_H_ */
