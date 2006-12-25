
#ifndef GETTER_H
#define GETTER_H

#include <string>
#include <exception>
#include "jnetlib/jnetlib.h"

using namespace std;


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


class HTTPMethodNotAllowedException: public HTTPException { 
    public:
        HTTPMethodNotAllowedException(const char* errstr): HTTPException(405,errstr) 
        { }
};



class HTTPMethod: public JNL_HTTPGet {
	private: 
	    int    reply;
		FILE*  fptr;
		string file;
	
    protected:
        string method;
        string url;
        int    len;
        
    public:
        HTTPMethod(string method, string url, string user="", string passwd="");
       	virtual ~HTTPMethod();
        
        void   addHeader(string header);              
        void   connect  ()                     throw (ConnectionException);
        int    read     (void* buf, int len)   throw (HTTPException);
        char*  readLine (char*& buf)           throw (HTTPException);
        
        int    contentLen();
        string contentType();
        long   lastModified();
        string lastModifiedStr();
};


class HTTPGet: public HTTPMethod {
    public:
        HTTPGet(string url, string user="", string passwd=""):
        	HTTPMethod("GET", url, user, passwd) {}
};


class HTTPPut: public HTTPMethod {
    public:
        HTTPPut(string url, string user="", string passwd="") throw (ConnectionException);
        
        int write(const void* bytes, int len);
};


class HTTPInfo: HTTPMethod {
    public:
        HTTPInfo(string url, string user="", string passwd="") throw (ConnectionException);
        
        // Promote these to public
        int    contentLen()			{ return HTTPMethod::contentLen();		}
        string contentType()		{ return HTTPMethod::contentType();		}
        long   lastModified()		{ return HTTPMethod::lastModified();	}
        string lastModifiedStr()	{ return HTTPMethod::lastModifiedStr();	}
};

#endif
