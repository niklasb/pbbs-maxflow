/*
 * GraphReader.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "GraphReader.h"

#include <fstream>
#include <stdlib.h>
#include "../network/Graph.h"
#include "../network/IOGraph.h"
#include <iostream>
using namespace std;

GraphReader::GraphReader() {
}

GraphReader::~GraphReader() {
}

Graph* GraphReader::readGraph(string& fileName) {
	reader.open(fileName.c_str());
	if (!reader.is_open()) {
		throw ios_base::failure("cannot open " + fileName);
	}
	int from, to;
	string line;
	// the first line contains the number of nodes in the graph
	getline(reader, line, '\n');
	Graph * g = new Graph(atoi(line.c_str()));
	// the next numberOfNodes lines contain the degree sequence
	for (int i = 0; i < g->n(); ++i) {
		getline(reader, line, '\t');
		from = atoi(line.c_str());
		getline(reader, line, '\n');
		to = atoi(line.c_str());
		if (!g->initAdjList(from, to)) {
			throw ios_base::failure(
					"cannot initialize adjacency list line" + (i + 2));
		}
	}
	// the remaining lines contain the adjacency list
	while (reader.good()) {
		getline(reader, line, '\t');
		from = atoi(line.c_str());
		getline(reader, line, '\n');
		to = atoi(line.c_str());
		if (from < to) {
			g->addEdge(from, to);
		}
	}
	reader.close();
	if (!g->isInitialized()) {
		throw ios_base::failure("graph has not been initialized");
	}
	return g;
}

Graph* GraphReader::readOldFormat(string& fileName) {
	reader.open(fileName.c_str());
	if (!reader.is_open()) {
		throw ios_base::failure("cannot open file");
	}
	int from, to;
	string line;
	// the first line contains the number of nodes in the graph
	getline(reader, line, '\n');
	IOGraph* ioGraph = new IOGraph(atoi(line.c_str()));
	while (reader.good()) {
		getline(reader, line, '\t');
		from = atoi(line.c_str());
		getline(reader, line, '\n');
		to = atoi(line.c_str());
		if (from < to) {
			ioGraph->addEdge(from, to);
		}
	}
	reader.close();
	Graph* g = new Graph(ioGraph);
	delete ioGraph;
	if (!g->isInitialized()) {
		throw ios_base::failure("graph has not been initialized");
	}
	return g;
}
