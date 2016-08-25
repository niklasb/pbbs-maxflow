/*
 * Forward.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef FORWARD_H_
#define FORWARD_H_

#include "../AbstractTriadAlgorithm.h"

class Forward: public AbstractTriadAlgorithm {
public:
	Forward(string name);
	~Forward();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	int* m_pi;
	int* m_invPi;

};

#endif /* FORWARD_H_ */
