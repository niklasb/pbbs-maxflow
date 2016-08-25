/*
 * DirectoryReader.cpp
 *
 *  Created on: 06.06.2013
 *      Author: ortmann
 */

#include "DirectoryReader.h"
#include "GraphReader.h"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <sstream>
#include <iostream>

DirectoryReader::DirectoryReader() :
		m_files(), m_path(), m_sortFiles(false), m_curPos(0) {
}

DirectoryReader::~DirectoryReader() {
}

void DirectoryReader::initFileList(string path, bool sortFiles) {
	m_curPos = 0;
	m_path = path;
	DIR *pDir;
	dirent* entry;
	struct stat buf;
	m_files.clear();
	if ((pDir = opendir(path.c_str()))) {
		while ((entry = readdir(pDir))) {
			lstat((path + entry->d_name).c_str(), &buf);
			if (S_ISREG(buf.st_mode)) {
				m_files.push_back(
						string(entry->d_name).substr(0,
								string(entry->d_name).find_last_of(".")));
			}
		}
		closedir(pDir);
	}

	// if (sortFiles) {
	// 	this->sortFiles();
	// }


}

Graph* DirectoryReader::getGraph() {
	return getGraph(false);
}

Graph* DirectoryReader::getGraph(bool newFormat) {
	GraphReader reader;
	string file(m_path + m_files[m_curPos]);

	if (newFormat) {
		return reader.readGraph(file);
	} else {
		return reader.readOldFormat(file);
	}
}

bool DirectoryReader::hasGraph() {
	return m_curPos < m_files.size();
}

bool DirectoryReader::hasNext() {
	return m_curPos != m_files.size() - 1;
}

void DirectoryReader::nextGraph() {
	++m_curPos;
}

string DirectoryReader::getPath() {
	return m_path;
}

string DirectoryReader::getGraphName() {
	return m_files[m_curPos];
}

void DirectoryReader::sortFiles() {
	vector<int> sortedList;
	for (unsigned int i = 0; i < m_files.size(); ++i) {
		sortedList.push_back(
				atoi(
						m_files[i].substr(0, m_files[i].find_last_of(".")).c_str()));
	}
	sort(sortedList.begin(), sortedList.end());
	m_files.clear();
	for (unsigned int i = 0; i < sortedList.size(); ++i) {
		stringstream s;
		s << sortedList[i];
		m_files.push_back(s.str());
	}
}
