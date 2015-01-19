/*
* SimpleTimer.h
*
* SimpleTimer - A timer library for Arduino.
* Author: mromani@ottotecnica.com
* Copyright (c) 2010 OTTOTECNICA Italy
*
* This library is free software; you can redistribute it
* and/or modify it under the terms of the GNU Lesser
* General Public License as published by the Free Software
* Foundation; either version 2.1 of the License, or (at
* your option) any later version.
*
* This library is distributed in the hope that it will
* be useful, but WITHOUT ANY WARRANTY; without even the
* implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*
* You should have received a copy of the GNU Lesser
* General Public License along with this library; if not,
* write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*
*/


#ifndef SIMPLETIMER_H
#define SIMPLETIMER_H

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

typedef void(*timer_callback)(void);

class SimpleTimer {

public:
	// maximum number of timers
	const static byte MAX_TIMERS = 3;

	// setTimer() constants
	const static byte RUN_FOREVER = 0;
	const static byte RUN_ONCE = 1;

	// constructor
	SimpleTimer();

	// this function must be called inside loop()
	void run();

	// call function f every d milliseconds
	byte setInterval(long d, timer_callback f);

	// call function f once after d milliseconds
	int setTimeout(long d, timer_callback f);

	// call function f every d milliseconds for n times
	byte setTimer(long d, timer_callback f, int n);

	// destroy the specified timer
	void deleteTimer(int numTimer);

	// restart the specified timer
	void restartTimer(int numTimer);

	// returns true if the specified timer is enabled
	boolean isEnabled(int numTimer);

	// enables the specified timer
	void enable(byte numTimer);

	// disables the specified timer
	void disable(byte numTimer);

	// enables the specified timer if it's currently disabled,
	// and vice-versa
	void toggle(int numTimer);

	// returns the number of used timers
	int getNumTimers();

	// returns the number of available timers
	int getNumAvailableTimers() { return MAX_TIMERS - numTimers; };

private:
	// deferred call constants
	const static byte DEFCALL_DONTRUN = 0;       // don't call the callback function
	const static byte DEFCALL_RUNONLY = 1;       // call the callback function but don't delete the timer
	const static byte DEFCALL_RUNANDDEL = 2;      // call the callback function and delete the timer

	// find the first available slot
	byte findFirstFreeSlot();

	// value returned by the millis() function
	// in the previous run() call
	unsigned long prev_millis[MAX_TIMERS];

	// pointers to the callback functions
	timer_callback callbacks[MAX_TIMERS];

	// delay values
	int delays[MAX_TIMERS];

	// number of executed runs for each timer
	byte numRuns[MAX_TIMERS];

	// which timers are enabled
	boolean enabled[MAX_TIMERS];

	// deferred function call (sort of) - N.B.: this array is only used in run()
	byte toBeCalled[MAX_TIMERS];

	// actual number of timers in use
	byte numTimers;
};

#endif