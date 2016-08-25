/*
 * ForwardHashed.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef FORWARDHASHED_H_
#define FORWARDHASHED_H_

#include "../AbstractTriadAlgorithm.h"
#include <tr1/unordered_set>

typedef std::tr1::unordered_set<int> hashSet;

class ForwardHashed: public AbstractTriadAlgorithm {
public:
	ForwardHashed(string name);
	~ForwardHashed();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	void intersect(hashSet& search, hashSet& lookup);
	int* m_pi;
	int* m_invPi;
};

#endif /* FORWARDHASHED_H_ */
