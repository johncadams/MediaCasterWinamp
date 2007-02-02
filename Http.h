#ifndef HTTP_H
#define HTTP_H

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
        
        ConnectionException(const char* cause, int errnum) {
        	string err = cause;
        	err += ": ";
        	err += strerror(errnum);
            ConnectionException::errstr = strdup(err.c_str());
        }

        const char* getError() {
            return errstr;
        }
        
        string toString() {
            return errstr;
        }
};


class CacheFileException: public ConnectionException {
	public:
        CacheFileException(const char* filename, int errnum): ConnectionException(filename,errnum) {}
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


class HTTPSession {
	private:
	    static int ref;
		string cachedir;
		string username;
		string passwd;
		
	public:
		HTTPSession(string cachedir, string username="", string passwd="");
		virtual ~HTTPSession();
		
		string getCacheDir()				{ return cachedir;			}
		string getUsername()				{ return username;			}
		string getPassword()				{ return passwd;			}
		
		void setUsername(const char* username);
		void setPassword(const char* passwd);
};



class HTTPMethod: public JNL_HTTPGet {
	private: 
	    int    reply;
		FILE*  fptr;		
	
    protected:
    	HTTPSession* session;
        string       method;
        string       url;
        string       file;
        int          len;
        
    public:
        HTTPMethod(string method, HTTPSession* session, string url, const char* file);
       	virtual ~HTTPMethod();
        
        void   addHeader(string header);              
        void   connect  ()                     throw (ConnectionException);
        int    read     (void* buf, int len)   throw (ConnectionException);
        char*  readLine (char*& buf)           throw (ConnectionException);
        
        int    contentLen();
        string contentType();
        long   lastModified();
        string lastModifiedStr();
};


class HTTPGet: public HTTPMethod {
	public:
        HTTPGet(HTTPSession* session, string url);
        HTTPGet(HTTPSession* session, string url, const char* file);
        string getCachedFile()		{ return file;	}
};


class HTTPPut: public HTTPMethod {
	public:
	    HTTPPut(HTTPSession* session, string url);
        int write(const void* bytes, int len);
};


class HTTPInfo: HTTPMethod {
	public:
	    HTTPInfo(HTTPSession* session, string url) throw (ConnectionException);
	    
        // Promote these to public
        int    contentLen()			{ return HTTPMethod::contentLen();		}
        string contentType()		{ return HTTPMethod::contentType();		}
        long   lastModified()		{ return HTTPMethod::lastModified();	}
        string lastModifiedStr()	{ return HTTPMethod::lastModifiedStr();	}
};

#endif
