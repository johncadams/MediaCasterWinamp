#include "Http.h"
#include "jnetlib/webserver.h"
#include "MediaCaster.h"
#include "Trace.h"
#include "date.h"


#define GET_METHOD			"GET"
#define HEAD_METHOD			"HEAD"
#define PUT_METHOD			"PUT"

#define OK					200
#define FILE_UPTODATE		304
#define FILE_NOT_FOUND		401
#define METHOD_NOT_ALLOWED	405

#define IF_MODIFIED_SINCE	"If-Modified-Since"
#define CONTENT_TYPE		"Content-Type"
#define LAST_MODIFIED		"Last-Modified"
#define WWW_AUTHENTICATE	"WWW-Authenticate"


HTTPMethod::HTTPMethod(string method, string url, string username, string password) {
    TRACE("HTTPMethod::HTTPMethod");
    HTTPMethod::url    = url;
    HTTPMethod::method = method;
    HTTPMethod::reply  = -1; 
    HTTPMethod::fptr   = NULL; 
    HTTPMethod::len    = 0;
    
    const char* ptr = NULL;
    for (const char* c=url.c_str(); *c; c++) {
    	if (*c=='/') ptr = c;
    }
    
    if (ptr) {
	    HTTPMethod::file = configuration.getCacheFile(++ptr);
	    int date = getFileDate(file.c_str());
	    if (date && method.compare(GET_METHOD)==0) {
	    	char* dateStr = getDateStr(date);
		    LOGGER(IF_MODIFIED_SINCE, dateStr);
		    string header(IF_MODIFIED_SINCE);
			header.append(": ");
			header.append(dateStr);
			addHeader(header);
	    }
    }
    
    if (username.length()>0 || password.length()>0) {
        char b64[256];
        string userpw = username +":"+ password;
        WebServerBaseClass::base64encode((char*)userpw.c_str(), b64);
        
        char hdr[256];
        sprintf(hdr, "Authorization: Basic %s", b64);
        addHeader(hdr);
    }
}
        
        
HTTPMethod::~HTTPMethod() {
	if (fptr) fclose(fptr);
	fptr = NULL;
}
        
        
void HTTPMethod::addHeader(string header) {
    TRACE("HTTPMethod::addHeader");
    JNL_HTTPGet::addheader(header.c_str());
}


int HTTPMethod::contentLen() {
	// TRACE("HTTPMethod::contentLen");
	if (len == 0) {
		if (reply==FILE_UPTODATE) len = getFileSize(file.c_str());		
		else                      len = JNL_HTTPGet::content_length();
	}
	return len;
}


string HTTPMethod::contentType() {
	// TRACE("HTTPMethod::contentType");
	return JNL_HTTPGet::getheader(CONTENT_TYPE);
} 


long HTTPMethod::lastModified() {
	// TRACE("HTTPMethod::lastModified");
	return getDate(lastModifiedStr().c_str() );
}


string HTTPMethod::lastModifiedStr() {
	// TRACE("HTTPMethod::lastModifiedStr");
	return JNL_HTTPGet::getheader(LAST_MODIFIED);
}


void HTTPMethod::connect() throw (ConnectionException) {
    TRACE("HTTPMethod::connect");
    LOGGER("URL", url.c_str());
    LOGGER("Method", method.c_str());
    JNL_HTTPGet::connect(url.c_str(), 0, (char*)method.c_str());
    
    int has_printed_headers = 0;
	int has_printed_reply   = 0;
    while (true) {	
    	int running = JNL_HTTPGet::run();
    	
	    if (running < 0) {
	    	reply = JNL_HTTPGet::getreplycode();
	    	LOGGER("ERROR", JNL_HTTPGet::geterrorstr());
	    	LOGGER("reply", JNL_HTTPGet::getreply());
			switch (reply) {
            	case FILE_UPTODATE: {
            		return;
            	}
                case FILE_NOT_FOUND: {
                    char realm[64];
                    char* hdr = JNL_HTTPGet::getheader(WWW_AUTHENTICATE);
                    if (hdr) sscanf(hdr, "Basic %s", realm);
                    THROW(HTTPAuthenticationException(JNL_HTTPGet::geterrorstr()));
                }
                case METHOD_NOT_ALLOWED:
                	THROW(HTTPMethodNotAllowedException(JNL_HTTPGet::geterrorstr()));
                	
                default:
                    THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
            }
	    }
	    
	    if (JNL_HTTPGet::get_status()>0) { // '1|'2' = reading headers|content
			if (!has_printed_reply) {
	        	has_printed_reply = 1;
	        	LOGGER("reply", JNL_HTTPGet::getreply());
	      	}
	      
	      	if (JNL_HTTPGet::get_status()==2) { // '2' = reading content	        	
	        	if (!has_printed_headers) {
	          		has_printed_headers = 1;
	          		char *p=JNL_HTTPGet::getallheaders();
	          		while (p&&*p) {
	          			LOGGER("headers", p);
	            		p+=strlen(p)+1;
	          		}
	        	}
	        	return;
	      	}
	    }

	    if (running == 1) { // 1 means connection closed
			LOGGER("DONE", "");
	      	break;
		}
 	}
}


int HTTPMethod::read(void* bytes, int len) throw (HTTPException) {
	if (reply==FILE_UPTODATE) {
		if (fptr==NULL) {
			TRACE("HTTPMethod::read");
			LOGGER("Reading cache", file.c_str());
			fptr = fopen(file.c_str(), "r");
			if (fptr==NULL) {
				fileIoProblemBox(plugin.hwndLibraryParent, file.c_str(), strerror(errno));
				exit(0);
			}
		}
		if (!fread(bytes, len, 1, fptr)) {
			fclose(fptr);
			fptr = NULL;
			return 0;
		} else {
			return len;
		}
	}
	
    while (true) {
        int running = JNL_HTTPGet::run();
                        
        if (running == -1) {
            THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
        }
        if (JNL_HTTPGet::get_status() == 2) {
            if (JNL_HTTPGet::bytes_available() > 0) {
                int num = JNL_HTTPGet::get_bytes((char*)bytes, len);
                if (fptr==NULL) {
                	TRACE("HTTPMethod::read");
                	LOGGER("Writing cache", file.c_str());
                	fptr = fopen(file.c_str(), "wb");
                	if (fptr==NULL) {
						fileIoProblemBox(plugin.hwndLibraryParent, file.c_str(), strerror(errno));
						exit(0);
					}
                }
                fwrite(bytes, num, 1, fptr);
                return num;
            }
        }
        
        if (running == 1) {
        	if (fptr) fclose(fptr);
        	fptr = NULL;
        	return 0;  // We're done
        }
    }
}


char* HTTPMethod::readLine(char*& buf) throw (HTTPException) {
    if (reply==FILE_UPTODATE) {
		if (fptr==NULL) {
			TRACE("HTTPMethod::read");
			LOGGER("Reading cache", file.c_str());
			fptr = fopen(file.c_str(), "r");
			if (fptr==NULL) {
				fileIoProblemBox(plugin.hwndLibraryParent, file.c_str(), strerror(errno));
				exit(0);
			}
		}
		char tmp[2048];
		
		if (!fgets(tmp, sizeof(tmp), fptr)) {
			TRACE("HTTPMethod::read");
            LOGGER("Closing", file.c_str());
			if (fptr) fclose(fptr);
			buf = NULL;
			fptr = NULL;
			return NULL;
		} else {
			buf = strdup(tmp);
			strtok(buf, "\n\r");
			return buf;
		}
	}
	
    while (true) {
        int running = JNL_HTTPGet::run();
                        
        if (running == -1) {
            THROW(HTTPException(JNL_HTTPGet::getreplycode(),JNL_HTTPGet::geterrorstr()));
        }
        if (JNL_HTTPGet::get_status()==2) {
            if (JNL_HTTPGet::bytes_available() > 0) {
                int   len = JNL_HTTPGet::bytes_available();
                buf = new char[len];
                
                JNL_HTTPGet::peek_bytes(buf,len);
                for (int i=0; i<len; i++) {
                    char chr = buf[i];
                    if (chr=='\n' || chr==EOF) {  // Windows: '\n\r'
                        memset(buf, 0, len);
                        int num = JNL_HTTPGet::get_bytes(buf, i+1);

                        if (fptr==NULL) {
                        	TRACE("HTTPMethod::read");
                        	LOGGER("Writing cache", file.c_str());
                        	fptr = fopen(file.c_str(), "wb");
                        	if (fptr==NULL) {
								fileIoProblemBox(plugin.hwndLibraryParent, file.c_str(), strerror(errno));
								exit(0);
							}
                        }
                        fwrite(buf, num, 1, fptr);
                        
                        if (buf[0]==EOF) break;  // This assumed an EOF is on its own line          
                        buf[--num] = 0;  // snip off the newline/EOF
                        if (buf[num-1]=='\r') buf[--num] = 0; // snip off the return
                        return buf;
                    }
                }
                delete buf;
                buf = NULL;
            }       
        }
        
        if (running == 1) {
        	TRACE("HTTPMethod::read");
            LOGGER("Closing", file.c_str());
        	if (fptr) fclose(fptr);
        	fptr = NULL;
        	return NULL;  // We're done
        }
    }
}


HTTPGet::HTTPGet(string url, string user, string passwd) :
	HTTPMethod(GET_METHOD, url, user, passwd) {
}


HTTPPut::HTTPPut(string url, string user, string passwd) :
    HTTPMethod(PUT_METHOD, url, user, passwd) {
}


int HTTPPut::write(const void* bytes, int len) {
	return m_con->send_bytes(bytes, len);
}


HTTPInfo::HTTPInfo(string url, string user, string passwd) throw (ConnectionException) :
    HTTPMethod(HEAD_METHOD, url, user, passwd) {
    TRACE("HTTPInfo::HTTPInfo");
    HTTPMethod::connect();
}
