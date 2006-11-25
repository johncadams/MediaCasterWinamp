
#ifndef GETTER_H
#define GETTER_H

#include <string>
#include <exception>
#include "jnetlib/jnetlib.h"

using namespace std;


long  getDate   (const char*);
char* getDateStr(struct tm*);
char* getDateStr(long);


class ConnectionException: public exception {
    private:
        const char* errstr;
        
    public:
        ConnectionException(const char* errstr) {
            ConnectionException::errstr = strdup(errstr?errstr:"");
        }

        const char* getError() {
            return errstr;
        }
        
        string toString() {
            return errstr;
        }
};


class HTTPException: public ConnectionException {
    private:
        int code;
        
    public:
        HTTPException(int code, const char* errstr): ConnectionException(errstr) {
            HTTPException::code = code;
        }
        
        int getErrorCode() {
            return code;
        }
        
        string toString() {
            char tmp[1024];
            sprintf(tmp, "%d: %s", getErrorCode(), getError());
            return string(tmp);
        }
};


class HTTPAuthenticationException: public HTTPException { 
    public:
        HTTPAuthenticationException(const char* errstr): HTTPException(401,errstr) 
        { }
};



class HTTPMethod: public JNL_HTTPGet {
    protected:
        string method;
        string url;                        
        
    public:
        HTTPMethod(string method, string url, string user="", string passwd="");
        
        void addHeader(string header);              
        void connect  ()                     throw (ConnectionException);
        int  read     (void* buf, int len)   throw (HTTPException);
        int  readLine (char*& buf)           throw (HTTPException);
        
        int  contentLen()   { return JNL_HTTPGet::content_length(); }
};


class HTTPGet: public HTTPMethod {
    public:
        HTTPGet(string url, string user="", string passwd=""):  HTTPMethod("GET", url, user, passwd) {}
};


class HTTPInfo: HTTPMethod {
    public:
        HTTPInfo(string url, string user="", string passwd="") throw (ConnectionException);
        
        int    contentLen  ()    { return HTTPMethod::contentLen();                }
        string contentType ()    { return JNL_HTTPGet::getheader("Content-Type");  } 
        long   lastModified()    { return getDate(lastModifiedStr().c_str() );     }
        string lastModifiedStr() { return JNL_HTTPGet::getheader("Last-Modified"); }
};

#endif
