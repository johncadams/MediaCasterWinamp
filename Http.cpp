
#include "HTTPGet.h"
#include "jnetlib/webserver.h"
#include "time.h"
#include "Trace.h"


HTTPMethod::HTTPMethod(string method, string url, string username, string password) {
    TRACE("HTTPMethod::HTTPMethod");
    HTTPMethod::url    = url;
    HTTPMethod::method = method;
    
    if (username.length()>0 || password.length()>0) {
        char b64[256];
        string userpw = username +":"+ password;
        WebServerBaseClass::base64encode((char*)userpw.c_str(), b64);
        
        char hdr[256];
        sprintf(hdr, "Authorization: Basic %s", b64);
        addHeader(hdr);
    }
}
        
        
void HTTPMethod::addHeader(string header) {
    TRACE("HTTPMethod::addHeader");
    JNL_HTTPGet::addheader(header.c_str());
}


void HTTPMethod::connect() throw (ConnectionException) {
    TRACE("HTTPMethod::connect");
    LOGGER("URL", url.c_str());
    JNL_HTTPGet::connect(url.c_str(), 0, (char*)method.c_str());
    
    while (true) {
        int running = JNL_HTTPGet::run();                  
        if (running == -1) {
            switch (JNL_HTTPGet::getreplycode()) {
                case 401: {
                    char realm[64];
                    char* hdr = JNL_HTTPGet::getheader("WWW-Authenticate");
                    if (hdr) sscanf(hdr, "Basic %s", realm);
                    THROW(HTTPAuthenticationException(JNL_HTTPGet::geterrorstr()));
                }
                case 0:
                    THROW(ConnectionException(JNL_HTTPGet::geterrorstr()));
                                    
                default:
                    THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
            }
        }

        switch (JNL_HTTPGet::get_status()) {
            case -1: {
                int code = JNL_HTTPGet::getreplycode();
                if (code==401) {THROW(HTTPAuthenticationException(JNL_HTTPGet::geterrorstr()));}
                else           {THROW(HTTPException         (code,JNL_HTTPGet::geterrorstr()));}
            }
            case  0:    Sleep(1);       break;      // still connecting            
            case  1:    return;                     // reading content            
            case  2:    return;                     // reading headers
        }
    }
}


int HTTPMethod::read(void* bytes, int len) throw (HTTPException) {
//  TRACE("HTTPMethod::read");
    while (true) {
        int running = JNL_HTTPGet::run();
                        
        if (running == -1) {
            TRACE("HTTPMethod::read");
            THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
        }
        if (JNL_HTTPGet::get_status() == 2) {
            if (JNL_HTTPGet::bytes_available() > 0) {
                int num = JNL_HTTPGet::get_bytes((char*)bytes, len);
                return num;
            }
        }
        
        if (running == 1) return 0;  // We're done
    }
}


int HTTPMethod::readLine(char*& buf) throw (HTTPException) {
//  TRACE("HTTPMethod::readLine");
    while (true) {
        int running = JNL_HTTPGet::run();
                        
        if (running == -1) {
            TRACE("HTTPMethod::readLine");
            THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
        }
        if (JNL_HTTPGet::get_status()==2) {
            if (JNL_HTTPGet::bytes_available() > 0) {
                int   len = JNL_HTTPGet::bytes_available();
                buf = new char[len];
                
                JNL_HTTPGet::peek_bytes(buf,len);
                for (int i=0; i<len; i++) {
                    char chr = buf[i];
                    if (chr=='\n' || chr==EOF) {
                        memset(buf, 0, len);
                        int num = JNL_HTTPGet::get_bytes(buf, i+1);
                        buf[--num] = 0;  // snip off the newline/EOF
                        
                        if (buf[num-1]=='\r') buf[--num] = 0; // snip off the return
                        return num;
                    }
                }
                delete buf;
            }       
        }
        
        if (running == 1) return 0;  // We're done
    }
}



HTTPInfo::HTTPInfo(string url, string user, string passwd) throw (ConnectionException) :
    HTTPMethod("HEAD", url, user, passwd) {
    TRACE("HTTPMethod::HTTPInfo");
    HTTPMethod::connect();
}


char* getDateStr(long time) {
    TRACE("getDateStr");
    struct tm* ctime = gmtime(&time);
    char*      str   = getDateStr(ctime);
    free(ctime);
    return str;
}


char* getDateStr(struct tm* ctime) {
    TRACE("getDateStr");
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

long getDate(const char* date) {
    TRACE("getDate");
    struct tm ctime;
    memset(&ctime, 0, sizeof(ctime));
    char   dow[4];
    char   mon[4];
    sscanf(date, "%3s, %2d %3s %4d %2d:%2d:%2d GMT", 
           dow, &(ctime.tm_mday), mon, &(ctime.tm_year),
           &(ctime.tm_hour), &(ctime.tm_min), &(ctime.tm_sec));
           
    ctime.tm_year -= 1900;
           
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
