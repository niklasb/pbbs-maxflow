/*
 * AlgorithmFactory.cpp
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#include "AlgorithmFactory.h"
#include "../algorithms/thesis/LON.h"
#include "../algorithms/thesis/T1O1.h"
#include "../algorithms/thesis/T2O1.h"
#include "../algorithms/thesis/LO1.h"
#include "../algorithms/thesis/LOM.h"
#include "../algorithms/thesis/SO1.h"
#include "../algorithms/thesis/SON.h"
#include "../algorithms/thesis/SONless.h"
#include "../algorithms/thesis/SOM.h"
#include "../algorithms/thesis/T1OM.h"
#include "../algorithms/thesis/T2OM.h"
#include "../algorithms/thesis/DegreeStatistic.h"

#include "../algorithms/related/EdgeIterator.h"
#include "../algorithms/related/EdgeIteratorHashed.h"
#include "../algorithms/related/Forward.h"
#include "../algorithms/related/CompactForward.h"
#include "../algorithms/related/K3.h"
#include "../algorithms/related/ForwardHashed.h"
#include "../algorithms/related/NodeIteratorSFO.h"
#include "../algorithms/related/NodeIterator.h"

AlgorithmFactory::AlgorithmFactory() {
}

AlgorithmFactory::~AlgorithmFactory() {
}

pair<string, vector<AbstractTriadAlgorithm*> > AlgorithmFactory::getAlgorithm(
		int algorithm) {
	vector<AbstractTriadAlgorithm*> vec;
	switch (algorithm) {
	case 0:
		lonFast(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("L-ON_fast.txt"), vec);
		break;
	case 1:
		lon(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("L-ON.txt"), vec);
		break;
	case 2:
		t1o1(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("T1-O1.txt"), vec);
		break;
	case 3:
		t2o1(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("T2-O1.txt"), vec);
		break;
	case 4:
		lo1_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("L-O1_merge.txt"), vec);
		break;
	case 5:
		lom_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("L-OM_merge.txt"), vec);
		break;
	case 6:
		so1_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("S1-O1_merge.txt"), vec);
		break;
	case 7:
		son_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("S1-ON_merge.txt"), vec);
		break;
	case 8:
		som_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("S1-OM_merge.txt"), vec);
		break;
	case 9:
		t1om_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("T1-OM_merge.txt"), vec);
		break;
	case 10:
		t2om_merged(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("T2-OM_merge.txt"), vec);
		break;
	case 11:
		son_less(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("S1-ON_less.txt"), vec);
		break;
	case 12:
		degStatistics(vec);
		return pair<string, vector<AbstractTriadAlgorithm*> >(
				string("Degree_Statistics.txt"), vec);
	default:
		throw "unsupported value";
	}
}

void AlgorithmFactory::lonFast(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new LON("ASC", OrderingFactory::ASC));
	vec.push_back(new LON("SFO", OrderingFactory::SFO));
}

void AlgorithmFactory::lon(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new LON("ASC", OrderingFactory::ASC));
	vec.push_back(new LON("DESC", OrderingFactory::DESC));
	vec.push_back(new LON("SFO", OrderingFactory::SFO));
	vec.push_back(new LON("BFO", OrderingFactory::BFO));
	vec.push_back(new LON("CO", OrderingFactory::CO));
}

void AlgorithmFactory::t1o1(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new T1O1("ASC", OrderingFactory::ASC));
	vec.push_back(new T1O1("DESC", OrderingFactory::DESC));
	vec.push_back(new T1O1("SFO", OrderingFactory::SFO));
	vec.push_back(new T1O1("BFO", OrderingFactory::BFO));
	vec.push_back(new T1O1("CO", OrderingFactory::CO));
}

void AlgorithmFactory::t2o1(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new T2O1("ASC", OrderingFactory::ASC));
	vec.push_back(new T2O1("DESC", OrderingFactory::DESC));
	vec.push_back(new T2O1("SFO", OrderingFactory::SFO));
	vec.push_back(new T2O1("BFO", OrderingFactory::BFO));
	vec.push_back(new T2O1("CO", OrderingFactory::CO));
}

void AlgorithmFactory::lo1_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new LO1("ASC", OrderingFactory::ASC));
	vec.push_back(new LO1("DESC", OrderingFactory::DESC));
	vec.push_back(new LO1("SFO", OrderingFactory::SFO));
	vec.push_back(new LO1("BFO", OrderingFactory::BFO));
	vec.push_back(new LO1("CO", OrderingFactory::CO));
	vec.push_back(new EdgeIterator("Edge-Iterator"));
}

void AlgorithmFactory::lom_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new LOM("ASC", OrderingFactory::ASC));
	vec.push_back(new LOM("DESC", OrderingFactory::DESC));
	vec.push_back(new LOM("SFO", OrderingFactory::SFO));
	vec.push_back(new LOM("BFO", OrderingFactory::BFO));
	vec.push_back(new LOM("CO", OrderingFactory::CO));
	vec.push_back(new EdgeIteratorHashed("Edge-Iterator-Hashed"));
}

void AlgorithmFactory::so1_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new SO1("ASC", OrderingFactory::ASC));
	vec.push_back(new SO1("DESC", OrderingFactory::DESC));
	vec.push_back(new SO1("SFO", OrderingFactory::SFO));
	vec.push_back(new SO1("BFO", OrderingFactory::BFO));
	vec.push_back(new SO1("CO", OrderingFactory::CO));
	vec.push_back(new Forward("Forward"));
	vec.push_back(new CompactForward("Compact-Forward"));
}

void AlgorithmFactory::son_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new SON("ASC", OrderingFactory::ASC));
	vec.push_back(new SON("DESC", OrderingFactory::DESC));
	vec.push_back(new SON("SFO", OrderingFactory::SFO));
	vec.push_back(new SON("BFO", OrderingFactory::BFO));
	vec.push_back(new SON("CO", OrderingFactory::CO));
	vec.push_back(new K3("K3"));
}

void AlgorithmFactory::som_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new SOM("ASC", OrderingFactory::ASC));
	vec.push_back(new SOM("DESC", OrderingFactory::DESC));
	vec.push_back(new SOM("SFO", OrderingFactory::SFO));
	vec.push_back(new SOM("BFO", OrderingFactory::BFO));
	vec.push_back(new SOM("CO", OrderingFactory::CO));
	vec.push_back(new ForwardHashed("Forward-Hashed"));
}

void AlgorithmFactory::t1om_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new T1OM("ASC", OrderingFactory::ASC));
	vec.push_back(new T1OM("DESC", OrderingFactory::DESC));
	vec.push_back(new T1OM("SFO", OrderingFactory::SFO));
	vec.push_back(new T1OM("BFO", OrderingFactory::BFO));
	vec.push_back(new T1OM("CO", OrderingFactory::CO));
	vec.push_back(new NodeIteratorSFO("Node-Iterator-SFO"));
}

void AlgorithmFactory::t2om_merged(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new T2OM("ASC", OrderingFactory::ASC));
	vec.push_back(new T2OM("DESC", OrderingFactory::DESC));
	vec.push_back(new T2OM("SFO", OrderingFactory::SFO));
	vec.push_back(new T2OM("BFO", OrderingFactory::BFO));
	vec.push_back(new T2OM("CO", OrderingFactory::CO));
	vec.push_back(new NodeIterator("Node-Iterator"));
}

void AlgorithmFactory::son_less(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new SON_less("ASC", OrderingFactory::ASC));
	vec.push_back(new SON_less("DESC", OrderingFactory::DESC));
	vec.push_back(new SON_less("SFO", OrderingFactory::SFO));
	vec.push_back(new SON_less("BFO", OrderingFactory::BFO));
	vec.push_back(new SON_less("CO", OrderingFactory::CO));
}

void AlgorithmFactory::degStatistics(vector<AbstractTriadAlgorithm*>& vec) {
	vec.push_back(new DegreeStatistic("ASC", OrderingFactory::ASC));
	vec.push_back(new DegreeStatistic("DESC", OrderingFactory::DESC));
	vec.push_back(new DegreeStatistic("SFO", OrderingFactory::SFO));
	vec.push_back(new DegreeStatistic("BFO", OrderingFactory::BFO));
	vec.push_back(new DegreeStatistic("CO", OrderingFactory::CO));
}
