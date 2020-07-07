#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "Timer.h"
#include "Task.h"

#define MAX_TASKS 10

class Scheduler {    
  private:
    int basePeriod;
    int nTasks;
    Task* taskList[MAX_TASKS];  
    Timer* timer;
  
  public:  
    Scheduler();
    void init(int basePeriod);  
    virtual bool addTask(Task* task);
    virtual void schedule();
};

#endif
