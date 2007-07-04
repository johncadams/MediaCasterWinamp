using namespace std;
#include <string>
#include <windows.h>

#include "gen_ml/ml.h"
#include "winamp/wa_dlg.h"

#include "Upgrade.h"
#include "Trace.h"
#include "Http.h"
#include "Process.h"
#include "MediaCaster.h"
#include "Messages.h"
#include "date.h"


Upgrade::Upgrade() {
    TRACE("Upgrade::Upgrade");    
    connProblem  = 0;
    installerUrl = configuration.getURL( configuration.getInstallerPath() );
}


void Upgrade::downloadFunction() throw (ConnectionException) {
    TRACE("Upgrade::downloadFunction");
    
    HTTPGet httpGet(httpSession,installerUrl);
    char    bytes[1024];
    int     cnt;
    int     total = 0;
    
    setStatusMessage(hwnd, CONNECTING);    
    httpGet.connect();    
    
    // Just kick the reader to generate the cached copy
    while( (cnt=httpGet.read(bytes, sizeof(bytes))) ) {
        total += cnt;
        char status[256];
        sprintf(status, INSTALLER_DOWNLOAD, int( float(total*100./httpGet.contentLen())) );
        setStatusMessage(hwnd, status);         
    }
    
	// Don't really need to do this since the installer closes winamp        
	// SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&plugin, ML_IPC_REMOVE_PLUGIN);
	
	string path    = httpGet.getCachedFile();
    string cmdline = path +" /S /D=" +configuration.getWinampDir();
    int    rtn     = execForegroundProcess(cmdline.c_str());

    remove(path.c_str());

    if (rtn==0) {
        LPVOID errmsg;
        DWORD  err = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,NULL,err,
                      MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&errmsg,0,NULL);
        LOGGER("ERROR", rtn);
        LOGGER("ERROR", err);
        LOGGER("ERROR", (char*)errmsg);
        exit(0);
        throw ConnectionException((char*)errmsg);
    } else {
		// The installer restarts winamp in silent mode
		// SendMessage(plugin.hwndWinampParent, WM_WA_IPC, 0, IPC_RESTARTWINAMP);
    }
}


void Upgrade::download() throw(ConnectionException) {
    TRACE("Upgrade::download");
    if (isAvailable()) {
        downloadFunction();
    }
}


int Upgrade::isAvailable() throw(ConnectionException) {
    TRACE("Upgrade::isAvailable");
    int status = false;
    
    try { 
		HTTPInfo httpInfo(httpSession,installerUrl);
		
        LOGGER("Local ", configuration.getBuildDate());
        LOGGER("Remote", httpInfo.lastModified());
        LOGGER("Local ", getDateStr(configuration.getBuildDate()));
        LOGGER("Remote", httpInfo.lastModifiedStr().c_str());
        LOGGER("Local ", getDate(getDateStr(configuration.getBuildDate())));
        LOGGER("Remote", getDate(httpInfo.lastModifiedStr().c_str()));
        if (httpInfo.lastModified()>configuration.getBuildDate()) {
            setStatusMessage(hwnd, UPGRADE_AVAIL_STATUS);
            status = true;
        } else {
            status = false;
        }        
        connProblem = 0;
        
    } catch (HTTPAuthenticationException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        RETHROW(ex);
        
    } catch (CacheFileException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        RETHROW(ex);
        
    } catch (HTTPException& ex) {
        if (ex.getErrorCode() == 404) {
            CATCH(ex);            
            if (!connProblem) {
            	connProblem = 1;
            	string reason = "file not found: "+installerUrl;
            	connectionProblemBox(hwnd, reason.c_str());
            }            
        } else {
            RETHROW(ex);
        }
    } catch (ConnectionException& ex) {
    	RETHROW(ex);
    }
    return status;
}


const char* Upgrade::getIsAvailableStatus() throw() {
    TRACE("Upgrade::getUpgradeAvailableMessage");
    
    try {
	    if (connProblem) {
	        return CONN_PROBLEM_STATUS2;        
	        
	    } else if (isAvailable()) {
	        return UPGRADE_AVAIL_STATUS;
	        
	    } else {
	        return UPTODATE_STATUS;
	    }
    } catch (ConnectionException& ex) {
    	CATCH(ex);
    	return CONN_PROBLEM_STATUS2;        
    }
}


void Upgrade::setHwnd(HWND hwnd) {
    TRACE("Upgrade::setHwnd");
    Upgrade::hwnd = hwnd;
}
