/*
 * OrderingFactory.cpp
 *
 *  Created on: 21.05.2013
 *      Author: ortmann
 */

#include "OrderingFactory.h"

#include "Asc.h"
#include "Desc.h"
#include "Co.h"
#include "Sfo.h"
#include "Bfo.h"

AbstractOrdering* OrderingFactory::getOrdering(SORTING sorting) {
	switch (sorting) {
	case ASC:
		return new Asc();
	case DESC:
		return new Desc();
	case CO:
		return new Co();
	case SFO:
		return new Sfo();
	case BFO:
		return new Bfo();
	default:
		return 0;
	}
}
