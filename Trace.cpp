#include "Trace.h"
#include "MediaCaster.h"


void Tracer::print(const char* marker, const char* message) {
    if (!tracerFd) {
        MessageBox(plugin.hwndLibraryParent, "Error opening log file", "Error", MB_OK);
        exit(0);    
    }
    for (int i=0; i<tracerDepth; i++) fprintf(tracerFd, " ");            
    fprintf(tracerFd, "%s%s\n", marker, message);
    fflush(tracerFd);   
}


Tracer::Tracer(const char* method) {
    Tracer::method = method;
    tracerDepth++;
    print("", method);
}

    
Tracer::~Tracer() {
    print("~", method);            
    tracerDepth--;
}


void Tracer::Logger(const char* msg, int num) {
    char tmp[512];
    sprintf(tmp, "%s:%d", msg, num);
    print("-", tmp);
}


void Tracer::Logger(const char* msg, const char* str) {
    char tmp[512];
    sprintf(tmp, "%s:%s%s%s", msg, str?"\"":"", str?str:"<NULL>", str?"\"":"");
    print("-", tmp);
}
