/*
 * EdgeIteratorHashed.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef EDGEITERATORHASHED_H_
#define EDGEITERATORHASHED_H_

#include "../AbstractTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class EdgeIteratorHashed: public AbstractTriadAlgorithm {
public:
	EdgeIteratorHashed(string name);
	~EdgeIteratorHashed();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	void intersect(hashSet& search, hashSet& lookup, int t1, int t2);
	hashSet* m_hGraph;
};

#endif /* EDGEITERATORHASHED_H_ */
