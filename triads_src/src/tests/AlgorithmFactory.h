/*
 * AlgorithmFactory.h
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#ifndef ALGORITHMFACTORY_H_
#define ALGORITHMFACTORY_H_

#include <string>
#include <vector>
#include <utility>
class AbstractTriadAlgorithm;

using namespace std;

class AlgorithmFactory {
public:
	AlgorithmFactory();
	~AlgorithmFactory();

	pair<string, vector<AbstractTriadAlgorithm*> > getAlgorithm(int algorithm);
private:
	void lonFast(vector<AbstractTriadAlgorithm*>& vec);
	void lon(vector<AbstractTriadAlgorithm*>& vec);
	void t1o1(vector<AbstractTriadAlgorithm*>& vec);
	void t2o1(vector<AbstractTriadAlgorithm*>& vec);
	void lo1_merged(vector<AbstractTriadAlgorithm*>& vec);
	void lom_merged(vector<AbstractTriadAlgorithm*>& vec);
	void so1_merged(vector<AbstractTriadAlgorithm*>& vec);
	void son_merged(vector<AbstractTriadAlgorithm*>& vec);
	void som_merged(vector<AbstractTriadAlgorithm*>& vec);
	void t1om_merged(vector<AbstractTriadAlgorithm*>& vec);
	void t2om_merged(vector<AbstractTriadAlgorithm*>& vec);
	void son_less(vector<AbstractTriadAlgorithm*>& vec);
	void degStatistics(vector<AbstractTriadAlgorithm*>& vec);

	AlgorithmFactory(const AlgorithmFactory& rhs);
	AlgorithmFactory& operator=(const AlgorithmFactory& rhs);
};

#endif /* ALGORITHMFACTORY_H_ */
