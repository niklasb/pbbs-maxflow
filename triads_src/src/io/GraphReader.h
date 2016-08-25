/*
 * GraphReader.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef GRAPHREADER_H_
#define GRAPHREADER_H_

#include <string>
#include <fstream>

class Graph;

using namespace std;

class GraphReader {
public:
	GraphReader();
	~GraphReader();

	Graph* readGraph(string& fileName);
	Graph* readOldFormat(string& fileName);

private:
	GraphReader(const GraphReader&);
	GraphReader& operator=(const GraphReader&);
	ifstream reader;
};

#endif /* GRAPHREADER_H_ */
