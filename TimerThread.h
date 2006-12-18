#ifndef TIMERTHREAD_H_
#define TIMERTHREAD_H_

#include <windows.h>


typedef unsigned long (WINAPI* function_type)(void*);
typedef void (*Procedure)(...);

class TimerThread {
    private:
        unsigned long id;
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
       unsigned long getId()                    { return id;        }
};

#endif /*TIMERTHREAD_H_*/
