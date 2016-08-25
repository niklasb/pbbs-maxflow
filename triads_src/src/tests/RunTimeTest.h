/*
 * RunTimeTest.h
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#ifndef RUNTIMETEST_H_
#define RUNTIMETEST_H_

#include <string>
#include <sys/time.h> //Julian added

#include "AlgorithmFactory.h"
#include "../io/DirectoryReader.h"

using namespace std;

class RunTimeTest {
public:
	RunTimeTest();
	~RunTimeTest();

	void runTests(int algorithm, string& path, bool newFormat, bool sortFiles);

private:
	void initDirs(string& path);
	void initWriters(ofstream& runTimeOrTriangleCountWriter,
			ofstream& runTimeAlgorithmOrOperationCountWriter,
			ofstream& runTimeOrderingOrAdjacencyCountWriter, string& path,
			pair<string, vector<AbstractTriadAlgorithm*> >& algorithms);
	void writeHeader(ofstream& output,
			vector<AbstractTriadAlgorithm*>& algorithms);
	bool writeTime(ofstream& output, timeval runTime);

	AlgorithmFactory m_factory;
	DirectoryReader m_dirReader;

	const string m_runTimeDir;
	const string m_orderingTimeDir;
	const string m_algorithmTimeDir;
	const string m_triangleCountDir;
	const string m_adjacencyCountDir;
	const string m_operationCountDir;
};

#endif /* RUNTIMETEST_H_ */
