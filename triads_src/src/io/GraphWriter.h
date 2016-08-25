/*
 * GraphWriter.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef GRAPHWRITER_H_
#define GRAPHWRITER_H_

#include <string>
#include <fstream>
// class forwarding
class IOGraph;

using namespace std;

class GraphWriter {
public:
	GraphWriter();
	~GraphWriter();

	bool writeGraph(const IOGraph* const g, string fileName) {
		return writeGraph(g, fileName, true);
	}
	bool writeGraphOldStyle(const IOGraph* const g, string fileName) {
		return writeGraph(g, fileName, false);
	}

private:
	bool writeGraph(const IOGraph* const g, string& fileName, bool newStyle);
	ofstream writer;
	// disable copy and assignment constructor
	GraphWriter(const GraphWriter&);
	GraphWriter& operator=(const GraphWriter&);
};

#endif /* GRAPHWRITER_H_ */
