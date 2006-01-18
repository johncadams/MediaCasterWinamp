#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>


#ifdef TRACE_GLOBALS
#ifdef DO_TRACING
    FILE*        tracerFd = fopen("C:/ml_mcaster.log", "w");    
#else
    FILE*        tracerFd;
#endif
    int          tracerDepth = 0;
#else 
    extern FILE* tracerFd;
    extern int   tracerDepth;
#endif



class Tracer {
    private:
        const char* method;
        
    protected:
        void print(const char* marker, const char* message);

    public:
        Tracer(const char* method);        
       ~Tracer();
        
        void Logger(const char* msg, int num);        
        void Logger(const char* msg, const char* str);
};


#ifdef DO_TRACING
    #define TRACE(method)   Tracer tRaCeR(method)
    #define LOGGER(msg,num) tRaCeR.Logger(msg,num)
    #define THROW(ex)       tRaCeR.Logger("Throw",       ex.toString().c_str()); throw ex;
    #define RETHROW(ex)     tRaCeR.Logger("Re-throwing", ex.toString().c_str()); throw ex;
    #define IGNOREX(ex,msg) tRaCeR.Logger(msg,           ex.toString().c_str())
    #define CATCH(ex)       tRaCeR.Logger("Catch",       ex.toString().c_str())
#else
    #define TRACE(method)
    #define LOGGER(msg,num)
    #define THROW(ex)
    #define RETHROW(ex)
    #define IGNOREX(ex,msg)
    #define CATCH(ex)
#endif


#endif /*TRACE_H*/
