#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>

 
class TracePrinter;
 
class TraceFrame {
	friend class TracePrinter;
	
    private:
    	TracePrinter* printer;
    	const char*   method;
     
    protected:
       TraceFrame(TracePrinter* printer, const char* method);
       
    public:         
       ~TraceFrame();
};



class TracePrinter {
	private:
		FILE* fd;
		char  tmp[4096];		
		int   inited;
    	int   depth;
		
	public:
		TracePrinter();		
	   ~TracePrinter();		
		
		void init(const char* logfile);
		
		TraceFrame getTraceFrame(const char* method);
		void       print(const char* marker, const char* message);
        void       log  (const char* msg, int num);        
        void       log  (const char* msg, const char* str);
        
        void       incr();
        void       decr();
};


#ifdef TRACE_GLOBALS
	TracePrinter tracePrinter;
#else
    extern TracePrinter tracePrinter;
#endif

#ifdef DO_TRACING
    #define TRACE(method)   TraceFrame tRaCeR = tracePrinter.getTraceFrame(method)
    #define LOGGER(msg,num) tracePrinter.log(msg,num)
    #define THROW(ex)       tracePrinter.log("Throw",       ex.toString().c_str()); throw ex;
    #define RETHROW(ex)     tracePrinter.log("Re-throwing", ex.toString().c_str()); throw;
    #define IGNOREX(ex,msg) tracePrinter.log(msg,           ex.toString().c_str())
    #define CATCH(ex)       tracePrinter.log("Catch",       ex.toString().c_str())
#else
    #define TRACE(method)
    #define LOGGER(msg,num)
    #define THROW(ex)                                                            throw ex;
    #define RETHROW(ex)                                                          throw;
    #define IGNOREX(ex,msg)
    #define CATCH(ex)
#endif


#endif /*TRACE_H*/
