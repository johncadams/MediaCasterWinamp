#include <stdlib.h>
#include <string.h>
#include "Trace.h"


TracePrinter::TracePrinter() {
	depth  = 0;
	inited = 0;
	tmp[0] = 0;
}

TracePrinter::~TracePrinter() {	
	if (fd && fd!=stdout && fd!=stderr) fclose(fd);
	fd = NULL;
}


int TracePrinter::init(const char* logfile, int isLogging) {
	if (!inited && isLogging) {
		if (strcmp(logfile,"stdout")==0) {
			fd = stdout;
			
		} else if (strcmp(logfile,"stderr")==0) {
			fd = stderr;
			
		} else if (strcmp(logfile,"stderr")==0) {
			fd = fopen(logfile, "w");
			if (fd) {
				fprintf(fd, "%s", tmp);
				
			} else {
				return errno;
			}		
		}
	}
	inited = 1;
	return 0;
}


TraceFrame TracePrinter::getTraceFrame(const char* method) {
	return TraceFrame(this, method);
}


void TracePrinter::print(const char* marker, const char* message) {
	if (fd) {
		for (int i=0; i<depth; i++) fprintf(fd, " ");            
	    fprintf(fd, "%s%s\n", marker, message);
	    fflush(fd);
	    
    } else if (!inited) {
    	for (int i=0; i<depth; i++) strcat(tmp, " ");            
	    strcat(tmp, marker);
	    strcat(tmp, message);
	    strcat(tmp, "\n");
	}
}


void TracePrinter::log(const char* msg, int num) {
    char tmp[512];
    sprintf(tmp, "%s:%d", msg, num);
    print("-", tmp);
}


void TracePrinter::log(const char* msg, const char* str) {
    char tmp[512];
    sprintf(tmp, "%s:%s%s%s", msg, str?"\"":"", str?str:"<NULL>", str?"\"":"");
    print("-", tmp);
}


void TracePrinter::incr() {
	depth++;
}


void TracePrinter::decr() {
	depth--;
}



TraceFrame::TraceFrame(TracePrinter* printer, const char* method) {
	TraceFrame::printer = printer;
    TraceFrame::method  = method;
    printer->incr();
    printer->print("", method);
}

    
TraceFrame::~TraceFrame() {
    printer->print("~", method);            
    printer->decr();
}
