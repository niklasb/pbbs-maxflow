/*
 * K3.h
 *
 *  Created on: 03.06.2013
 *      Author: ortmann
 */

#ifndef K3_H_
#define K3_H_

#include "../AbstractTriadAlgorithm.h"

class DLGraph;

class K3: public AbstractTriadAlgorithm {
public:
	K3(string name);
	~K3();
protected:
	void prepare(Graph* const g);
	void countTriads(Graph* const g);
private:
	DLGraph* m_dlGraph;
};

#endif /* K3_H_ */
