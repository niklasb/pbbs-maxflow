/*
 * NodeIteratorSFO.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef NODEITERATORSFO_H_
#define NODEITERATORSFO_H_

#include "../AbstractTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class DLGraph;

class NodeIteratorSFO: public AbstractTriadAlgorithm {
public:
	NodeIteratorSFO(string name);
	~NodeIteratorSFO();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	DLGraph* m_dlGraph;
	hashSet* m_hGraph;
};

#endif /* NODEITERATORSFO_H_ */
