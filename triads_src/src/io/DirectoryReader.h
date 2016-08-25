/*
 * DirectoryReader.h
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#ifndef DIRECTORYREADER_H_
#define DIRECTORYREADER_H_

#include <string>
#include <vector>

class Graph;

using namespace std;
class DirectoryReader {
public:
	DirectoryReader();
	~DirectoryReader();

	void initFileList(string path, bool sortFiles);
	Graph* getGraph();
	Graph* getGraph(bool newFormat);
	bool hasGraph();
	bool hasNext();
	void nextGraph();
	string getPath();
	string getGraphName();
private:
	vector<string> m_files;
	string m_path;
	bool m_sortFiles;
	unsigned int m_curPos;

	void sortFiles();
	DirectoryReader(const DirectoryReader&);
	DirectoryReader& operator=(const DirectoryReader&);
};

#endif /* DIRECTORYREADER_H_ */
