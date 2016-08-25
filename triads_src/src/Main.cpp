/*
 * Main.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

#include "tests/SyntheticDataGenerator.h"
#include "tests/RunTimeTest.h"

using namespace std;

int main(int lenght, char ** args) {
//	plot(string(args[1]));
//	string orkut_argo(
//			"/home/ortmann/master_thesis_tests/stanford/cit-Patents.txt");
//	testGraphSort (orkut);
//	SyntheticDataGenerator gen;
//	gen.create(string(args[1]));

	vector<string> dirs;
	string path(args[1]);

//	dirs.push_back(string(path + "pref_25_fixed/"));
//	dirs.push_back(string(path + "pref_50_fixed/"));
//	dirs.push_back(string(path + "pref_75_fixed/"));
//
//	dirs.push_back(string(path + "pref_25_dyn/"));
//	dirs.push_back(string(path + "pref_50_dyn/"));
//	dirs.push_back(string(path + "pref_75_dyn/"));

//	dirs.push_back(string(path + "torus_12_fixed/"));
//	dirs.push_back(string(path + "torus_11_fixed/"));
//	dirs.push_back(string(path + "torus_21_fixed/"));

//	dirs.push_back(string(path + "torus_12_dyn/"));
//	dirs.push_back(string(path + "torus_11_dyn/"));
//	dirs.push_back(string(path + "torus_21_dyn/"));

	dirs.push_back(string(path));
	RunTimeTest test;
	int selection = atoi(args[2]);
	for (unsigned int i = 0; i < dirs.size(); ++i) {
		string curDir = dirs[i];
		cout << "running test " << curDir << " with selection " << selection
				<< endl;
		test.runTests(selection, curDir, false, true);
	}
//	string stanford = path + "stanford/";
//	test.runTests(selection, stanford, false, true);
	return 0;
}
