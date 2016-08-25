/*
 * PrefAttTriaClosure.h
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#ifndef PREFATTTRIACLOSURE_H_
#define PREFATTTRIACLOSURE_H_

#include "AbstractGenerator.h"

class PrefAttTriaClosure: public AbstractGenerator {
public:
	PrefAttTriaClosure();
	~PrefAttTriaClosure();
	void setDegree(int degree);
	void setTriadFormProb(double prob);
protected:
	IOGraph* generate(int numOfNodes);

private:
	int m_outDegree;
	double m_triadFormationProb;
};

#endif /* PREFATTTRIACLOSURE_H_ */
