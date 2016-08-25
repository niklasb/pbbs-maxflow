/*
 * AbstractTriadAlgorithm.h
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#ifndef ABSTRACTTRIADALGORITHM_H_
#define ABSTRACTTRIADALGORITHM_H_

#include "../measurement/StopWatch.h"
#include <string>
#include <iostream>
//Julian commented out
//#define TIME

using namespace std;

// class forwarding
class Graph;

class AbstractTriadAlgorithm {
public:
	virtual ~AbstractTriadAlgorithm();

	const string& getName() const;

	timeval getRuntimeAlgorithm();

	timeval getRuntimeOrdering();

	timeval getRuntimeTotal();

	inline void runAlgorithm(Graph* const graph) {
		m_numOfTriangles = 0;
#ifndef TIME
		resetValues();
#endif
		m_timeMeasure.startOrdMeasuring();
		prepare(graph);
		m_timeMeasure.stopOrdStartAlgo();
		countTriads(graph);
		m_timeMeasure.stopAlgoMeasuring();
	}
	unsigned long long getTriangleCount();

#ifndef TIME

	unsigned long long getOperationCount();

	unsigned long long getAdjRequestCount();

#endif

protected:
	AbstractTriadAlgorithm(string name);
	virtual void prepare(Graph* const graph) = 0;
	virtual void countTriads(Graph* const graph)= 0;

	unsigned long long m_numOfTriangles;

#ifndef TIME
	unsigned long long m_numOfOperations;
	unsigned long long m_numOfAdjRequest;

#endif

private:
	const string m_algoName;
	StopWatch m_timeMeasure;

#ifndef TIME
	void resetValues();
#endif
};

#endif /* ABSTRACTTRIADALGORITHM_H_ */
