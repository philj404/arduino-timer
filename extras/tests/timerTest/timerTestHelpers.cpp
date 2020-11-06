//
// timerTestHelpers.cpp
//
// Confirm arduino-timer behaves as expected.

// UnixHostDuino emulation needs this include
// (it's not picked up "for free" by Arduino IDE)
//
#include <Arduino.h>

// fake it for UnixHostDuino emulation
#if defined(UNIX_HOST_DUINO)
#  ifndef ARDUINO
#  define ARDUINO 100
#  endif
#endif

//
// also, you need to provide your own forward references

// These tests depend on the Arduino "AUnit" library
//#include <AUnit.h>
//#include <arduino-timer.h>
#include "timerTestHelpers.h"

////////////////////////////////////////////////////////
// Simulate delays and elapsed time
//
unsigned long simulateTime::simTime = 0;
unsigned long simulateTime::simMillis(void) {
  return simTime;
}
void simulateTime::simDelay(unsigned long delayTime) {
  simTime += delayTime;
}

// TRAP ambiguous calls to delay() and millis() that are NOT simulated
//
void simulateTime::delay(unsigned long delayTime) {
  simDelay(delayTime);
}
unsigned long simulateTime::millis(void) {
  return simMillis();
}


////////////////////////////////////////////////////////
// instrumented dummy tasks
DummyTask::DummyTask(int runTime, bool repeats):
  busyTime(runTime), repeat(repeats)
{
  reset();
};

bool DummyTask::run(void) {
  timeOfLastRun = simMillis();
  simDelay(busyTime);
  numRuns++;
  return repeat;
};

// run a task as an object method
bool DummyTask::runATask(void * aDummy)
{
  DummyTask * myDummy = static_cast<DummyTask *>(aDummy);
  return myDummy->run();
};

void DummyTask::reset(void) {
  timeOfLastRun = 0;
  numRuns = 0;
};

// trivial dummy task to perform
bool no_op(void *) {
  return false;
}

