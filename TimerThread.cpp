
#include "TimerThread.h"
#include "Trace.h"


unsigned long WINAPI run(void* params) {
    TRACE("TimerThread::run");
    TimerThread* that = (TimerThread*)params;
    while (that->isRunning()) {
        that->callProcedure();
        if (!that->getWait()) break;
        Sleep(that->getWait());
    }
    that->setStopped();
    return 0;
}


TimerThread::TimerThread(unsigned long wait, Procedure procedure, void* args) {
    TRACE("TimerThread::TimerThread");
    TimerThread::wait      = wait;
    TimerThread::procedure = procedure;
    TimerThread::args      = args;  
    TimerThread::running   = 0;      
}


TimerThread::~TimerThread() {
}


void TimerThread::start() {
    TRACE("TimerThread::start");
    if (!running) {
        running = 1;
        CreateThread(NULL,  // security
                     0,     // stack size
                     run,   // start
                     this,  // parameters
                     0,     // creation flags
                     &id);
    }
}


void TimerThread::stop() {
    TRACE("TimerThread::stop");
    running = 0;
    WaitForSingleObject((HANDLE)id, INFINITE);
}
