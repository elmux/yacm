/**
 * Create timer objects
 *
 * @file    timer.c
 * @version 0.1
 * @author  Elmar Vonlanthen (vonle1@bfh.ch)
 * @date    May 26, 2011
 */

#include <stdlib.h>
#include <sys/time.h>
#include "defines.h"
#include "timer.h"

typedef struct {
	unsigned long startTime;
	unsigned long endTime;
} TimerDescriptor;

TIMER setUpTimer(unsigned int time) {
	TimerDescriptor *timerDescriptor = malloc(sizeof(TimerDescriptor));
	struct timeval tv;

	//if (timerDescriptor == NULL) {
	//	return NULL;
	//}
	// get current time as timeval structure:
	gettimeofday(&tv, NULL);

	// get time in milliseconds and save it as start time:
	timerDescriptor->startTime = (tv.tv_sec*1000) + (tv.tv_usec/1000);
	// set end time:
	timerDescriptor->endTime = timerDescriptor->startTime + time;
	// return structure:
	return timerDescriptor;
}

int tearDownTimer(TIMER timer) {
	if (timer == NULL) {
		return FALSE;
	}

	free(timer);
	return TRUE;
}
int isTimerElapsed(TIMER timer) {
	struct timeval tv;
	unsigned long curTime;

	if (timer == NULL) {
		return FALSE;
	}

	TimerDescriptor *td = timer;

	// get current time as timeval structure:
	gettimeofday(&tv, NULL);

	// get time in milliseconds and save it as start time:
	curTime = (tv.tv_sec*1000) + (tv.tv_usec/1000);
	// check if timer is elapsed:
	if (curTime >= td->endTime) {
		free(td);
		return TRUE;
	}
	return FALSE;
}
