#ifndef TIMERTHREAD_H_
#define TIMERTHREAD_H_

#include <windows.h>


typedef void (*Procedure)(...);

class TimerThread {
    private:
        unsigned      thread;
        unsigned long wait;
        Procedure     procedure;
        void*         args;
        int           running;
        
    public:
	   TimerThread(unsigned long, Procedure, void*);
	   virtual ~TimerThread();
       
       void start();
       void stop ();
       
       unsigned long getWait()                  { return wait;      }
       void          callProcedure()            { procedure(args);  }
       int           isRunning()                { return running;   }
       void          setStopped()               { running = 0;      }
       unsigned      getThread()				{ return thread;	}
};

#endif /*TIMERTHREAD_H_*/
