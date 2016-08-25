/*
 * AbstractGenerator.h
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#ifndef ABSTRACTGENERATOR_H_
#define ABSTRACTGENERATOR_H_

class IOGraph;

class AbstractGenerator {
public:
	virtual ~AbstractGenerator();
	IOGraph* generateGraph(int numOfNodes);

protected:

	virtual IOGraph* generate(int numOfNodes) = 0;
	AbstractGenerator();
	double randDouble() const;

private:
	void permute(IOGraph* g);
	void makeConnected(IOGraph* g) const;
	// disable copy and assignment constructor
	AbstractGenerator(const AbstractGenerator&);
	AbstractGenerator& operator=(const AbstractGenerator&);

};

#endif /* ABSTRACTGENERATOR_H_ */
