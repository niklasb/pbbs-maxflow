/*
 * OrderingFactory.h
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#ifndef ORDERINGFACTORY_H_
#define ORDERINGFACTORY_H_

// class forwarding
class AbstractOrdering;

class OrderingFactory {
public:
	enum SORTING {
		ASC, DESC, CO, SFO, BFO
	};

	static AbstractOrdering* getOrdering(SORTING sorting);

private:
	OrderingFactory();
	~OrderingFactory();
	OrderingFactory(const OrderingFactory&);
	OrderingFactory& operator=(const OrderingFactory&);
};

#endif /* ORDERINGFACTORY_H_ */
