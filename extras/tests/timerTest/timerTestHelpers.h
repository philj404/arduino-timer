//
// timerTestHelpers.h
//
// UnixHostDuino emulation needs this include
// (it's not picked up "for free" by Arduino IDE)
//
#ifndef TIMERTESTHELPERS_H
#define TIMERTESTHELPERS_H

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
#include <AUnit.h>
#include <arduino-timer.h>

////////////////////////////////////////////////////////
// You really want to be simulating time, rather than
// forcing tests to slow down.
// This simulation works across different processor families.
//
// BE CAREFUL to use delay() and millis() ONLY WHEN YOU MEAN IT!
//
// this namespace collision should help you make it more clear what you get
//
namespace simulateTime {

////////////////////////////////////////////////////////////////////////////////
// Simulate delays and elapsed time
//
extern unsigned long simTime;

unsigned long simMillis(void);
void simDelay(unsigned long delayTime);

// TRAP ambiguous calls to delay() and millis() that are NOT simulated
//
void delay(unsigned long delayTime);
unsigned long millis(void);

}; // namespace simulateTime

using namespace simulateTime; // and detect ambiguous function calls


////////////////////////////////////////////////////////////////////////////////
// instrumented dummy tasks
class DummyTask {
  public:
    DummyTask(int runTime, bool repeats = true);

    unsigned long busyTime;
    int numRuns;
    unsigned long timeOfLastRun;
    bool repeat;

    bool run(void); // consume some "time"

    // run a task as an object method
    static bool runATask(void * aDummyTask);

    // reset task count
    void reset(void);
};

// trivial dummy task; instantly returns false.
//
bool no_op(void *);

#endif // TIMERTESTHELPERS_H
