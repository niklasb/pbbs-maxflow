/*
 * SyntheticDataGenerator.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef SYNTHETICDATAGENERATOR_H_
#define SYNTHETICDATAGENERATOR_H_

#include <string>

class AbstractGenerator;
class IOGraph;
#include "../generators/AbstractGenerator.h"
#include "../generators/TorusGNP.h"
#include "../generators/PrefAttTriaClosure.h"

using namespace std;

class SyntheticDataGenerator {
public:
	SyntheticDataGenerator();
	virtual ~SyntheticDataGenerator();
	void create(string path, bool fix);
private:
	void createPref(string& path, bool fix);
	void createTorus(string& path, bool fix);
	void print(IOGraph* g, string& path, int avg);
	int calcDynAvgDeg(int numOfNodes);
	void setTorus(TorusGNP& gen, int numOfNodes, int avgDegree,
			double tradeOff);
	void createFixed(AbstractGenerator* gen, string& path, int avgDegree);
	void createDyn(AbstractGenerator* gen, string& path, int avgDegree,
			int nodeCount);
	// disable copy and assignment constructor
	SyntheticDataGenerator(const SyntheticDataGenerator&);
	SyntheticDataGenerator& operator=(const SyntheticDataGenerator&);
};

#endif /* SYNTHETICDATAGENERATOR_H_ */
