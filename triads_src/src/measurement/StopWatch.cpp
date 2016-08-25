/*
 * TimeMeasurment.cpp
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#include "StopWatch.h"

StopWatch::StopWatch() {
}

StopWatch::~StopWatch() {
}

timeval StopWatch::getOrderingTime() {
#ifdef TOD
	return getTimeDiff(stopOrdTimeVal, startOrdTimeVal);
#else
	return addTimeVals(
			getTimeDiff(stopOrdTimeUsage.ru_utime, startOrdTimeUsage.ru_utime),
			getTimeDiff(stopOrdTimeUsage.ru_stime, startOrdTimeUsage.ru_stime));
#endif
}

timeval StopWatch::getAlgoTime() {
#ifdef TOD
	return getTimeDiff(stopAlgoTimeVal, startAlgoTimeVal);
#else
	return addTimeVals(
			getTimeDiff(stopAlgoTimeUsage.ru_utime,
					startAlgoTimeUsage.ru_utime),
			getTimeDiff(stopAlgoTimeUsage.ru_stime,
					startAlgoTimeUsage.ru_stime));
#endif
}

timeval StopWatch::getTotalTime() {
	return addTimeVals(getOrderingTime(), getAlgoTime());
}

timeval StopWatch::getTimeDiff(timeval& stop, timeval& start) {
	timeval elapsedTime;
	timersub(&stop, &start, &elapsedTime);
	return elapsedTime;
}

timeval StopWatch::addTimeVals(timeval time1, timeval time2) {
	timeval totalElapsedTime;
	timeradd(&time1, &time2, &totalElapsedTime);
	return totalElapsedTime;
}
