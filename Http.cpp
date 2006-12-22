#include <sys/stat.h>
#include <time.h>

#include "HTTPGet.h"
#include "jnetlib/webserver.h"
#include "MediaCaster.h"
#include "Trace.h"

#define OK					200
#define FILE_UPTODATE		304
#define FILE_NOT_FOUND		401

#define IF_MODIFIED_SINCE	"If-Modified-Since"
#define CONTENT_TYPE		"Content-Type"
#define LAST_MODIFIED		"Last-Modified"
#define WWW_AUTHENTICATE	"WWW-Authenticate"


static char* getDateStr(struct tm* ctime) {
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


static long getDate(const char* date) {
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


static int getFileDate(const char* filename) {
    // TRACE("getFileDate");
    struct stat buf;
    if (stat(filename, &buf)==0) {
    	return buf.st_mtime;
    }
    return 0;
}


static int getFileSize(const char* filename) {
    // TRACE("getFileSize");
    struct stat buf;
    if (stat(filename, &buf)==0) {
    	return buf.st_size;
    }
    return 0;
}


static char* getDateStr(time_t time) {
    // TRACE("getDateStr");
    struct tm* ctime = gmtime(&time);
    char*      str   = getDateStr(ctime);
    return str;
}



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
	    if (date) {
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
        
        
HTTPMethod::~HTTPMethod() 
{ }
        
        
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
	TRACE("HTTPMethod::contentType");
	return JNL_HTTPGet::getheader(CONTENT_TYPE);
} 


long HTTPMethod::lastModified() {
	TRACE("HTTPMethod::lastModified");
	return getDate(lastModifiedStr().c_str() );
}


string HTTPMethod::lastModifiedStr() {
	TRACE("HTTPMethod::lastModifiedStr");
	return JNL_HTTPGet::getheader(LAST_MODIFIED);
}


void HTTPMethod::connect() throw (ConnectionException) {
    TRACE("HTTPMethod::connect");
    LOGGER("URL", url.c_str());
    JNL_HTTPGet::connect(url.c_str(), 0, (char*)method.c_str());
    
    while (true) {
        int running = JNL_HTTPGet::run();                  
        if (running == -1) {
        	reply = JNL_HTTPGet::getreplycode();
            switch (reply) {
            	case FILE_UPTODATE: {
            		break;
            	}
                case FILE_NOT_FOUND: {
                    char realm[64];
                    char* hdr = JNL_HTTPGet::getheader(WWW_AUTHENTICATE);
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
            	reply = JNL_HTTPGet::getreplycode();
            	if (reply==FILE_UPTODATE)  { return; }
                if (reply==FILE_NOT_FOUND) { THROW(HTTPAuthenticationException(JNL_HTTPGet::geterrorstr())); }
                else                       { THROW(HTTPException(reply,JNL_HTTPGet::geterrorstr())); }
            }
            case  0:    Sleep(1);       break;      // still connecting            
            case  1:    return;                     // reading content            
            case  2:    return;                     // reading headers
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
                	fptr = fopen(file.c_str(), "w");
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


int HTTPMethod::readLine(char*& buf) throw (HTTPException) {
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
			fclose(fptr);
			buf = NULL;
			fptr = NULL;
			return 0;
		} else {
			buf = strdup(tmp);
			return strlen(tmp);
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
                    if (chr=='\n' || chr==EOF) {
                        memset(buf, 0, len);
                        int num = JNL_HTTPGet::get_bytes(buf, i+1);
                        
                        if (fptr==NULL) {
                        	TRACE("HTTPMethod::read");
                        	LOGGER("Writing cache", file.c_str());
                        	fptr = fopen(file.c_str(), "w");
                        	if (fptr==NULL) {
								fileIoProblemBox(plugin.hwndLibraryParent, file.c_str(), strerror(errno));
								exit(0);
							}
                        }
                        fwrite(buf, num, 1, fptr);
                        fflush(fptr);
                        buf[--num] = 0;  // snip off the newline/EOF                        
                        if (buf[num-1]=='\r') buf[--num] = 0; // snip off the return
                        return num;
                    }
                }
                delete buf;
            }       
        }
        
        if (running == 1) {
        	if (fptr) fclose(fptr);
        	fptr = NULL;
        	buf = NULL;
        	return 0;  // We're done
        }
    }
}



HTTPInfo::HTTPInfo(string url, string user, string passwd) throw (ConnectionException) :
    HTTPMethod("HEAD", url, user, passwd) {
    TRACE("HTTPMethod::HTTPInfo");
    HTTPMethod::connect();
}
