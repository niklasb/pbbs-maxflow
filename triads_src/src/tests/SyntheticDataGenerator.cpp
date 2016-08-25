/*
 * SyntheticDataGenerator.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "SyntheticDataGenerator.h"

#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <math.h>

#include "../network/IOGraph.h"
#include "../io/GraphWriter.h"

const static unsigned int FIXED_RETRIES = 4;
const static unsigned int FIXED_NODE_COUNT = 250000;
const static unsigned int MIN_FIXED_AVG_DEG = 6;
const static unsigned int MAX_FIXED_AVG_DEG = 66;
const static unsigned int FIXED_STEP_SIZE = 6;

const static unsigned int DYN_RETRIES = 4;
const static unsigned int MIN_DYN_NODE_COUNT = 500000;
const static unsigned int MAX_DYN_NODE_COUNT = 5000000;
const static unsigned int DYN_STEP_SIZE = 900000;
const static unsigned int MIN_DYN_AVG_DEG = 6;
const static unsigned int MAX_DYN_AVG_DEG = 36;

SyntheticDataGenerator::SyntheticDataGenerator() {
}

SyntheticDataGenerator::~SyntheticDataGenerator() {
}

void SyntheticDataGenerator::create(string path, bool fix) {
	if (fix) {
		srand(42);
	} else {
		srand(29121985);
	}
	createPref(path, fix);
	createTorus(path, fix);
}

void SyntheticDataGenerator::createPref(string& path, bool fix) {
	PrefAttTriaClosure pref;
	if (fix) {
		string twentyfiveFixed(path + "pref_25_fixed");
		mkdir(twentyfiveFixed.c_str(), S_IRWXU);

		string fiftyFixed(path + "pref_50_fixed");
		mkdir(fiftyFixed.c_str(), S_IRWXU);

		string seventyfiveFixed(path + "pref_75_fixed");
		mkdir(seventyfiveFixed.c_str(), S_IRWXU);
		for (unsigned int i = MIN_FIXED_AVG_DEG; i <= MAX_FIXED_AVG_DEG; i +=
				FIXED_STEP_SIZE) {
			pref.setDegree(i / 2);
			pref.setTriadFormProb(0.25);
			createFixed(&pref, twentyfiveFixed, i);
			pref.setTriadFormProb(0.5);
			createFixed(&pref, fiftyFixed, i);
			pref.setTriadFormProb(0.75);
			createFixed(&pref, seventyfiveFixed, i);
		}
	} else {
		string twentyfiveDyn(path + "pref_25_dyn");
		mkdir(twentyfiveDyn.c_str(), S_IRWXU);

		string fiftyDyn(path + "pref_50_dyn");
		mkdir(fiftyDyn.c_str(), S_IRWXU);

		string seventyfiveDyn(path + "pref_75_dyn");
		mkdir(seventyfiveDyn.c_str(), S_IRWXU);

		for (unsigned int nodeCount = MIN_DYN_NODE_COUNT;
				nodeCount <= MAX_DYN_NODE_COUNT; nodeCount += DYN_STEP_SIZE) {
			int avgDegree = calcDynAvgDeg(nodeCount);
			pref.setDegree(avgDegree / 2);
			pref.setTriadFormProb(0.25);
			createDyn(&pref, twentyfiveDyn, avgDegree, nodeCount);
			pref.setTriadFormProb(0.5);
			createDyn(&pref, fiftyDyn, avgDegree, nodeCount);
			pref.setTriadFormProb(0.75);
			createDyn(&pref, seventyfiveDyn, avgDegree, nodeCount);
		}
	}
}

void SyntheticDataGenerator::createTorus(string& path, bool fix) {
	TorusGNP torusGnp;
	if (fix) {
		string torus1gnp2Fixed(path + "torus_12_fixed");
		mkdir(torus1gnp2Fixed.c_str(), S_IRWXU);

		string torus1gnp1Fixed(path + "torus_11_fixed");
		mkdir(torus1gnp1Fixed.c_str(), S_IRWXU);

		string torus2gnp1Fixed(path + "torus_21_fixed");
		mkdir(torus2gnp1Fixed.c_str(), S_IRWXU);
		for (unsigned int i = MIN_FIXED_AVG_DEG; i <= MAX_FIXED_AVG_DEG; i +=
				FIXED_STEP_SIZE) {
			setTorus(torusGnp, FIXED_NODE_COUNT, i, 1 / 3.0);
			createFixed(&torusGnp, torus1gnp2Fixed, i);
			setTorus(torusGnp, FIXED_NODE_COUNT, i, 0.5);
			createFixed(&torusGnp, torus1gnp1Fixed, i);
			setTorus(torusGnp, FIXED_NODE_COUNT, i, 2 / 3.0);
			createFixed(&torusGnp, torus2gnp1Fixed, i);
		}
	} else {
		string torus1gnp2Dyn(path + "torus_12_dyn");
		mkdir(torus1gnp2Dyn.c_str(), S_IRWXU);

		string torus1gnp1Dyn(path + "torus_11_dyn");
		mkdir(torus1gnp1Dyn.c_str(), S_IRWXU);

		string torus2gnp1Dyn(path + "torus_21_dyn");
		mkdir(torus2gnp1Dyn.c_str(), S_IRWXU);

		for (unsigned int nodeCount = MIN_DYN_NODE_COUNT;
				nodeCount <= MAX_DYN_NODE_COUNT; nodeCount += DYN_STEP_SIZE) {
			int avgDeg = calcDynAvgDeg(nodeCount);
			setTorus(torusGnp, nodeCount, avgDeg, 1 / 3.0);
			createDyn(&torusGnp, torus1gnp2Dyn, avgDeg, nodeCount);
			setTorus(torusGnp, nodeCount, avgDeg, 0.5);
			createDyn(&torusGnp, torus1gnp1Dyn, avgDeg, nodeCount);
			setTorus(torusGnp, nodeCount, avgDeg, 2 / 3.0);
			createDyn(&torusGnp, torus2gnp1Dyn, avgDeg, nodeCount);
		}
	}
}

void SyntheticDataGenerator::setTorus(TorusGNP& gen, int numOfNodes,
		int avgDegree, double tradeOff) {
	int radius = ceil(avgDegree * tradeOff / 2);
	gen.setRadius(radius);
	double remDeg = avgDegree - 2 * radius;
	double prob = remDeg / (numOfNodes - 1.0);
	gen.setProb(prob);
}

void SyntheticDataGenerator::print(IOGraph* g, string& path, int avg) {
	cout << path << " we want " << avg << " we generated "
			<< (2.0 * g->m() / g->n()) << endl;
}

int SyntheticDataGenerator::calcDynAvgDeg(int numOfNodes) {
	const double dist = MAX_DYN_NODE_COUNT - MIN_DYN_NODE_COUNT;
	if (dist == 0) {
		return MIN_DYN_AVG_DEG;
	}
	return MIN_DYN_AVG_DEG
			+ (numOfNodes - MIN_DYN_NODE_COUNT) / dist
					* (MAX_DYN_AVG_DEG - MIN_DYN_AVG_DEG);
}

void SyntheticDataGenerator::createFixed(AbstractGenerator* gen, string& path,
		int avgDegree) {
	GraphWriter writer;
	for (unsigned int j = 0; j < FIXED_RETRIES; ++j) {
		IOGraph* g = gen->generateGraph(FIXED_NODE_COUNT);
		print(g, path, avgDegree);
		stringstream s;
		s << path << "/" << avgDegree << j << ".txt";
		cout << s.str() << endl;
		writer.writeGraphOldStyle(g, s.str());
		delete g;
	}
}

void SyntheticDataGenerator::createDyn(AbstractGenerator* gen, string& path,
		int avgDegree, int nodeCount) {
	GraphWriter writer;
	for (unsigned int j = 0; j < DYN_RETRIES; ++j) {
		IOGraph* g = gen->generateGraph(nodeCount);
		print(g, path, avgDegree);
		stringstream s;
		s << path << "/" << nodeCount << j << ".txt";
		writer.writeGraphOldStyle(g, s.str());
		delete g;
	}
}
