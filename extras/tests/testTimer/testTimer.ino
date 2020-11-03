
//not needed: #line 2 "testTimer.ino"
// 
// testTimer.ino
//
// Confirm arduino-timer behaves as expected.


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

////////////////////////////////////////////////////////
// Simulate delays and elapsed time
//
unsigned long simTime = 0;
unsigned long simMillis(void) {
  return simTime;
}
void simDelay(unsigned long delayTime) {
  simTime += delayTime;
}

// TRAP ambiguous calls to delay() and millis() that are NOT simulated
void delay(unsigned long delayTime) {
  simDelay(delayTime);
}
unsigned long millis(void) {
  return simMillis();
}

}; // namespace simulateTime
using namespace simulateTime; // detect ambiguous function calls


////////////////////////////////////////////////////////
// instrumented dummy tasks
class DummyTask {
  public:
    DummyTask(int runTime, bool repeats = true):
      busyTime(runTime), repeat(repeats)
    {
      reset();
    };

    int busyTime;
    int numRuns;
    int timeOfLastRun;
    bool repeat;

    bool run(void) {
      timeOfLastRun = simMillis();
      simDelay(busyTime);
      numRuns++;
      return repeat;
    };

    // run a task as an object method
    static bool runATask(void * aDummy)
    {
      DummyTask * myDummy = static_cast<DummyTask *>(aDummy);
      return myDummy->run();
    };

    void reset(void) {
      timeOfLastRun = 0;
      numRuns = 0;
    };
};

// trivial dummy task to perform
bool no_op(void *) {
  return false;
}

// timer doesn't work as a local var; too big for the stack?
// Since it's not on the stack it's harder to guarantee it starts empty after the first test()
#define MAXTASKS 5
Timer<MAXTASKS, simMillis> timer;

void prepForTests(void) {
  timer.cancelAll();
  simTime = 0;
}

//////////////////////////////////////////////////////////////////////////////
// confirm tasks can be cancelled.
test(timer_cancelTasks) {
  prepForTests();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // no tasks!

  DummyTask dt_3millisec(3);
  DummyTask dt_5millisec(5);
  DummyTask dt_7millisec(7);

  auto inTask = timer.in(13, DummyTask::runATask, &dt_3millisec);
  auto atTask = timer.at(17, DummyTask::runATask, &dt_5millisec);
  auto everyTask = timer.every(19, DummyTask::runATask, &dt_7millisec);

  aWait = timer.ticks();
  assertEqual(aWait, 13); // inTask delay

  timer.cancel(inTask);
  aWait = timer.ticks();
  assertEqual(aWait, 17); // atTask delay

  timer.cancel(atTask);
  aWait = timer.ticks();
  assertEqual(aWait, 19); // everyTask delay

  timer.cancel(everyTask);
  aWait = timer.ticks();
  assertEqual(aWait, 0); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.at() behaviors
//
test(timer_at) {
  prepForTests();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // no tasks!
  assertEqual(simMillis(), 0ul);

  DummyTask waste_3ms(3);

  const int atTime = 17;
  const int lateStart = 4;
  simDelay(lateStart);

  auto atTask = timer.at(atTime, DummyTask::runATask, &waste_3ms);

  aWait = timer.tick();
  assertEqual(aWait, atTime - lateStart);
  assertEqual(waste_3ms.numRuns, 0);

  for (int i = lateStart + 1; i < atTime; i++ ) {
    simDelay(1);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 0);  // still waiting
  }

  simDelay(1);
  aWait = timer.tick();
  assertEqual(waste_3ms.numRuns, 1);  // triggered
  assertEqual(aWait, 0);

  simDelay(1);
  aWait = timer.tick();
  assertEqual(waste_3ms.numRuns, 1);  // not repeating

  aWait = timer.tick();
  assertEqual(aWait, 0); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.in() behaviors
//
test(timer_in) {
  prepForTests();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // no tasks!
  assertEqual(simMillis(), 0ul);

  DummyTask waste_3ms(3);

  const int lateStart = 7;
  simDelay(lateStart);

  const int delayTime = 17;
  auto atTask = timer.in(delayTime, DummyTask::runATask, &waste_3ms);

  aWait = timer.tick();
  assertEqual(aWait, delayTime);
  assertEqual(waste_3ms.numRuns, 0);

  for (int i = 1; i < delayTime; i++ ) {
    simDelay(1);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 0);  // still waiting
  }

  simDelay(1);
  aWait = timer.tick();
  assertEqual(waste_3ms.numRuns, 1);  // triggered
  assertEqual(aWait, 0);

  simDelay(1);
  aWait = timer.tick();
  assertEqual(waste_3ms.numRuns, 1);  // not repeating

  aWait = timer.tick();
  assertEqual(aWait, 0); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.every() behaviors
//
test(timer_every) {
  prepForTests();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // no tasks!
  assertEqual(simMillis(), 0ul);

  DummyTask waste_3ms(3);
  DummyTask waste_100ms_once(100, false);

  const int lateStart = 7;
  simDelay(lateStart);

  //const int delayTime = 17;
  auto everyTask1 = timer.every(50, DummyTask::runATask, &waste_3ms);
  auto everyTask2 = timer.every(200, DummyTask::runATask, &waste_100ms_once);

  aWait = timer.tick();
  assertEqual(aWait, 50);
  assertEqual(waste_3ms.numRuns, 0);

  for (int i = 1; i < 1000; i++ ) {
    simDelay(1);
    aWait = timer.tick();
  }

  assertEqual(waste_3ms.numRuns, 22);  // triggered
  assertEqual(waste_100ms_once.numRuns, 1);  // triggered

  aWait = timer.tick();
  assertEqual(aWait, 39); // still a repeating task
};

//////////////////////////////////////////////////////////////////////////////
// confirm calculated delays to next event in the timer are "reasonable"
// reported by timer.tick() and timer.ticks().
//
test(timer_delayToNextEvent) {
  prepForTests();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // no tasks!

  DummyTask dt_3millisec(3);
  DummyTask dt_5millisec(5);
  timer.every( 7, DummyTask::runATask, &dt_3millisec);
  timer.every(11, DummyTask::runATask, &dt_5millisec);

  assertEqual(dt_3millisec.numRuns, 0);

  int start = simMillis();
  assertEqual(start, 0);  // earliest task

  aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 7);  // earliest task

  aWait = timer.tick();   // no tasks ran?
  int firstRunTime = simMillis() - start;
  assertEqual(firstRunTime, 0);

  simDelay(aWait);
  int firstActiveRunStart = simMillis();
  aWait = timer.tick();
  int firstTaskRunTime = simMillis() - firstActiveRunStart;
  assertEqual(firstTaskRunTime, 3);
  assertEqual(aWait, (11 - 7 - 3)); // other pending task

  // run some tasks; count them.
  while (simMillis() < start + 1000) {
    aWait = timer.tick();
    simDelay(aWait);
  }

  // expect the other task causes some missed deadlines
  assertNear(dt_3millisec.numRuns, 100, 9); // 7+ millisecs apart (ideally 142 runs)
  assertNear(dt_5millisec.numRuns, 90, 4); // 11+ millisecs apart (ideally 90 runs)
};


////////////// ... so which sketch is this?
void showID(void)
{
  Serial.println();
  Serial.println(F( "Running " __FILE__ ", Built " __DATE__));
};

//////////////////////////////////////////////////////////////////////////////
void setup() {
  ::delay(1000); // wait for stability on some boards to prevent garbage Serial
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // for the Arduino Leonardo/Micro only
  showID();
}

//////////////////////////////////////////////////////////////////////////////
void loop() {
  // Should get:
  // TestRunner summary:
  //    <n> passed, <n> failed, <n> skipped, <n> timed out, out of <n> test(s).
  aunit::TestRunner::run();
}