#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "Trace.h"
#include "date.h"


char* getDateStr(struct tm* ctime) {
    // TRACE("getDateStr");
    char dow[4], mon[4];
    char tmp[256];
    switch (ctime->tm_wday) {
        case 0: strcpy(dow, "Sun"); break;
        case 1: strcpy(dow, "Mon"); break;
        case 2: strcpy(dow, "Tue"); break;
        case 3: strcpy(dow, "Wed"); break;
        case 4: strcpy(dow, "Thu"); break;
        case 5: strcpy(dow, "Fri"); break;
        case 6: strcpy(dow, "Sat"); break;
    }
    
    switch(ctime->tm_mon) {
        case  0: strcpy(mon, "Jan"); break;
        case  1: strcpy(mon, "Feb"); break;
        case  2: strcpy(mon, "Mar"); break;
        case  3: strcpy(mon, "Apr"); break;
        case  4: strcpy(mon, "May"); break;
        case  5: strcpy(mon, "Jun"); break;
        case  6: strcpy(mon, "Jul"); break;
        case  7: strcpy(mon, "Aug"); break;
        case  8: strcpy(mon, "Sep"); break;
        case  9: strcpy(mon, "Oct"); break;
        case 10: strcpy(mon, "Nov"); break;
        case 11: strcpy(mon, "Dec"); break;
    }
    
    sprintf(tmp, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
            dow, ctime->tm_mday, mon, 1900+ctime->tm_year, 
            ctime->tm_hour, ctime->tm_min, ctime->tm_sec);

    return strdup(tmp);
}


time_t getDate(const char* date) {
    // TRACE("getDate");
    struct tm ctime;
    memset(&ctime, 0, sizeof(ctime));
    char   dow[4];
    char   mon[4];
    char   tzn[8];
    sscanf(date, "%3s, %2d %3s %4d %2d:%2d:%2d %s", 
           dow, &(ctime.tm_mday), mon, &(ctime.tm_year),
           &(ctime.tm_hour), &(ctime.tm_min), &(ctime.tm_sec),
           tzn);
           
    ctime.tm_year -= 1900;
    ctime.tm_isdst = -1; // Let it figure our DST
    if (strcmp(tzn,"GMT")==0) {    	
    	ctime.tm_hour -= 7; // Assume MST
    }
           
    if      (strcmp(mon,"Jan")==0) ctime.tm_mon =  0;
    else if (strcmp(mon,"Feb")==0) ctime.tm_mon =  1;
    else if (strcmp(mon,"Mar")==0) ctime.tm_mon =  2;
    else if (strcmp(mon,"Apr")==0) ctime.tm_mon =  3;
    else if (strcmp(mon,"May")==0) ctime.tm_mon =  4;
    else if (strcmp(mon,"Jun")==0) ctime.tm_mon =  5;
    else if (strcmp(mon,"Jul")==0) ctime.tm_mon =  6;
    else if (strcmp(mon,"Aug")==0) ctime.tm_mon =  7;
    else if (strcmp(mon,"Sep")==0) ctime.tm_mon =  8;
    else if (strcmp(mon,"Oct")==0) ctime.tm_mon =  9;
    else if (strcmp(mon,"Nov")==0) ctime.tm_mon = 10;
    else if (strcmp(mon,"Dec")==0) ctime.tm_mon = 11;
           
    return mktime(&ctime);
}


time_t getFileDate(const char* filename) {
    // TRACE("getFileDate");
    struct stat buf;
    if (stat(filename, &buf)==0) {
    	return buf.st_mtime;
    }
    return 0;
}


int getFileSize(const char* filename) {
    // TRACE("getFileSize");
    struct stat buf;
    if (stat(filename, &buf)==0) {
    	return buf.st_size;
    }
    return 0;
}


char* getDateStr(time_t time) {
    // TRACE("getDateStr");
    struct tm* ctime = gmtime(&time);
    char*      str   = getDateStr(ctime);
    return str;
}
