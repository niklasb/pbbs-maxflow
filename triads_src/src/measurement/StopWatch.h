/*
 * StopWatch.h
 *
 *  Created on: 20.05.2013
 *      Author: ortmann
 */

#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#define TOD

#include <sys/time.h>
#ifndef TOD
#include <sys/resource.h>
#endif

class StopWatch {
public:
	StopWatch();
	~StopWatch();
	inline void startOrdMeasuring() {
#ifdef TOD
		getTOD(&startOrdTimeVal);
#else
		getRusage(&startOrdTimeUsage);
#endif
	}

	inline void stopOrdMeasuring() {
#ifdef TOD
		getTOD(&stopOrdTimeVal);
#else
		getRusage(&stopOrdTimeUsage);
#endif
	}

	inline void startAlgoMeasuring() {
#ifdef TOD
		getTOD(&startAlgoTimeVal);
#else
		getRusage(&startAlgoTimeUsage);
#endif
	}

	inline void stopAlgoMeasuring() {
#ifdef TOD
		getTOD(&stopAlgoTimeVal);
#else
		getRusage(&stopAlgoTimeUsage);
#endif
	}

	inline void stopOrdStartAlgo() {
		stopOrdMeasuring();
		startAlgoMeasuring();
	}

	timeval getOrderingTime();

	timeval getAlgoTime();

	timeval getTotalTime();

private:

	timeval getTimeDiff(timeval& stop, timeval& start);
	timeval addTimeVals(timeval time1, timeval time2);

#ifdef TOD
	timeval startOrdTimeVal;
	timeval stopOrdTimeVal;
	timeval startAlgoTimeVal;
	timeval stopAlgoTimeVal;

	void getTOD(timeval* time) {
		gettimeofday(time, 0);
	}
#else
	rusage startOrdTimeUsage;
	rusage stopOrdTimeUsage;
	rusage startAlgoTimeUsage;
	rusage stopAlgoTimeUsage;

	void getRusage(rusage* time) {
		getrusage(RUSAGE_SELF, time);
	}
#endif

	// Disable copy and copy assignment
	StopWatch(const StopWatch& toCopy);
	StopWatch& operator=(const StopWatch);

};

#endif /* STOPWATCH_H_ */
