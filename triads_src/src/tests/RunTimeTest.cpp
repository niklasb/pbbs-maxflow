/*
 * RunTimeTest.cpp
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#include "RunTimeTest.h"
#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../algorithms/AbstractTriadAlgorithm.h"
#include "../network/Graph.h"

#define TIME 1

const static unsigned int MAX_TIME = 900000;

#ifdef TIME
const static unsigned int RETRIES = 5;
#else
const static unsigned int RETRIES = 1;
#endif

RunTimeTest::RunTimeTest() :
		m_factory(), //
		m_dirReader(), //
		m_runTimeDir("runTime/"), //
		m_orderingTimeDir("runTimeOrdering/"), //
		m_algorithmTimeDir("runTimeAlgorithm/"), //
		m_triangleCountDir("triangleCount/"), //
		m_adjacencyCountDir("adjacencyCount/"), //
		m_operationCountDir("operationCount/") {
}

RunTimeTest::~RunTimeTest() {
}

void RunTimeTest::runTests(int algorithm, string& path, bool newFormat,
		bool sortFiles) {
#ifdef TIME
	timeval defTime;
	defTime.tv_sec = MAX_TIME;
	defTime.tv_usec = 0;
#endif
	initDirs(path);
	m_dirReader.initFileList(path, sortFiles);
	pair<string, vector<AbstractTriadAlgorithm*> > algoPair =
			m_factory.getAlgorithm(algorithm);
	ofstream runTimeOrTriangleCountWriter;
	ofstream runTimeAlgorithmOrOperationCountWriter;
	ofstream runTimeOrderingOrAdjacencyCountWriter;
	initWriters(runTimeOrTriangleCountWriter,
			runTimeAlgorithmOrOperationCountWriter,
			runTimeOrderingOrAdjacencyCountWriter, path, algoPair);
	vector<AbstractTriadAlgorithm*>& algorithms = algoPair.second;
	AbstractTriadAlgorithm* algo;
	
#ifdef TIME
	vector<bool> toRun;
	for (unsigned int i = 0; i < algorithms.size(); ++i) {
		toRun.push_back(true);
	}
#endif
	while (m_dirReader.hasGraph()) {
		// read the graph
		Graph* ioGraph = m_dirReader.getGraph(newFormat);
		for (unsigned int i = 1; i <= RETRIES; ++i) {
			runTimeOrTriangleCountWriter << m_dirReader.getGraphName() << "\t";
			runTimeAlgorithmOrOperationCountWriter << m_dirReader.getGraphName()
					<< "\t";
			runTimeOrderingOrAdjacencyCountWriter << m_dirReader.getGraphName()
					<< "\t";
			for (unsigned int j = 1; j <= algorithms.size(); ++j) {
				algo = algorithms[j - 1];
#ifdef TIME
				if (toRun[j - 1]) {
#endif
					Graph* g = ioGraph->clone();
					algo->runAlgorithm(g);
					delete g;
#ifdef TIME
					writeTime(runTimeOrTriangleCountWriter,
							algo->getRuntimeTotal());
					timeval runTime = algo->getRuntimeTotal();

					toRun[j - 1] = writeTime(
							runTimeAlgorithmOrOperationCountWriter,
							algo->getRuntimeAlgorithm());
					writeTime(runTimeOrderingOrAdjacencyCountWriter,
							algo->getRuntimeOrdering());
#else
					runTimeOrTriangleCountWriter << algo->getTriangleCount();
					runTimeAlgorithmOrOperationCountWriter
					<< algo->getOperationCount();
					runTimeOrderingOrAdjacencyCountWriter
					<< algo->getAdjRequestCount();
#endif
#ifdef TIME
				} else {
					writeTime(runTimeOrTriangleCountWriter, defTime);
					writeTime(runTimeAlgorithmOrOperationCountWriter, defTime);
					writeTime(runTimeOrderingOrAdjacencyCountWriter, defTime);
				}
#endif
				if (j != algorithms.size()) {
					runTimeOrTriangleCountWriter << "\t";
					runTimeAlgorithmOrOperationCountWriter << "\t";
					runTimeOrderingOrAdjacencyCountWriter << "\t";
				} else {
					if (m_dirReader.hasNext() || i != RETRIES) {
						runTimeOrTriangleCountWriter << endl;
						runTimeAlgorithmOrOperationCountWriter << endl;
						runTimeOrderingOrAdjacencyCountWriter << endl;
					} else {
						runTimeOrTriangleCountWriter.flush();
						runTimeAlgorithmOrOperationCountWriter.flush();
						runTimeOrderingOrAdjacencyCountWriter.flush();
						runTimeOrTriangleCountWriter.close();
						runTimeAlgorithmOrOperationCountWriter.close();
						runTimeOrderingOrAdjacencyCountWriter.close();
					}
				}
			}
		}
		delete ioGraph;
		m_dirReader.nextGraph();
	}

	for (unsigned int i = 0; i < algorithms.size(); ++i) {
		delete algorithms[i];
	}
}

void RunTimeTest::initDirs(string& path) {
#ifdef TIME
	mkdir((path + m_runTimeDir).c_str(), S_IRWXU);
	mkdir((path + m_orderingTimeDir).c_str(), S_IRWXU);
	mkdir((path + m_algorithmTimeDir).c_str(), S_IRWXU);
#else
	mkdir((path + m_triangleCountDir).c_str(), S_IRWXU);
	mkdir((path + m_adjacencyCountDir).c_str(), S_IRWXU);
	mkdir((path + m_operationCountDir).c_str(), S_IRWXU);
#endif
}

void RunTimeTest::initWriters(ofstream& runTimeOrTriangleCountWriter,
		ofstream& runTimeAlgorithmOrOperationCountWriter,
		ofstream& runTimeOrderingOrAdjacencyCountWriter, string& path,
		pair<string, vector<AbstractTriadAlgorithm*> >& algorithms) {
#ifdef TIME
	runTimeOrTriangleCountWriter.open(
			(path + m_runTimeDir + algorithms.first).c_str());
	runTimeAlgorithmOrOperationCountWriter.open(
			(path + m_algorithmTimeDir + algorithms.first).c_str());
	runTimeOrderingOrAdjacencyCountWriter.open(
			(path + m_orderingTimeDir + algorithms.first).c_str());
#else
	runTimeOrTriangleCountWriter.open(
			(path + m_triangleCountDir + algorithms.first).c_str());
	runTimeAlgorithmOrOperationCountWriter.open(
			(path + m_operationCountDir + algorithms.first).c_str());
	runTimeOrderingOrAdjacencyCountWriter.open(
			(path + m_adjacencyCountDir + algorithms.first).c_str());
#endif
	writeHeader(runTimeOrTriangleCountWriter, algorithms.second);
	writeHeader(runTimeAlgorithmOrOperationCountWriter, algorithms.second);
	writeHeader(runTimeOrderingOrAdjacencyCountWriter, algorithms.second);
}

void RunTimeTest::writeHeader(ofstream& output,
		vector<AbstractTriadAlgorithm*>& algorithms) {
	output << RETRIES << "\t";
	unsigned int i = 0;
	for (; i < algorithms.size() - 1; ++i) {
		output << algorithms[i]->getName() << "\t";
	}
	output << algorithms[i]->getName() << endl;
}

bool RunTimeTest::writeTime(ofstream& output, timeval runTime) {
	output << runTime.tv_sec << "." << std::setw(6) << std::setfill('0')
			<< setiosflags(std::ios::right) << runTime.tv_usec;
	return (runTime.tv_sec < MAX_TIME);
}
