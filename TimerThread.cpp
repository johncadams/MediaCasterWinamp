#include <process.h>

#include "TimerThread.h"
#include "Trace.h"


unsigned __stdcall run(void* params) {
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
    TimerThread::thread    = 0;
    TimerThread::wait      = wait;
    TimerThread::procedure = procedure;
    TimerThread::args      = args;  
    TimerThread::running   = 0;      
}


TimerThread::~TimerThread() {
	_endthreadex(thread);
}


void TimerThread::start() {
    TRACE("TimerThread::start");
    if (!running) {
        running = 1;
        thread = _beginthreadex(
        			NULL,  // security
                    0,     // stack size
                    run,   // start
                    this,  // parameters
                    0,     // creation flags
                    NULL);
    }
}


void TimerThread::stop() {
    TRACE("TimerThread::stop");
    switch (WaitForSingleObject((void*)thread, 50000)) {
		case WAIT_OBJECT_0:
    		//  The thread has terminated - do something
    		break;
    
		case WAIT_TIMEOUT:
    		//  The timeout has elapsed but the thread is still running
    		//  do something appropriate for a timeout
    		break;
	}
}
