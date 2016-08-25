/*
 * TorusGNP.h
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#ifndef TORUSGNP_H_
#define TORUSGNP_H_

#include "AbstractGenerator.h"

class TorusGNP: public AbstractGenerator {
public:
	TorusGNP();
	~TorusGNP();
	void setRadius(int radius);
	void setProb(double prob);
protected:
	IOGraph* generate(int numOfNodes);

private:
	TorusGNP(const TorusGNP&);
	TorusGNP& operator=(const TorusGNP&);
	int m_radius;
	double m_prob;
};

#endif /* TORUSGNP_H_ */
