using namespace std;
#include <string>
#include <windows.h>

#include "gen_ml/ml.h"
#include "winamp/wa_dlg.h"

#include "Upgrade.h"
#include "Trace.h"
#include "HTTPGet.h"
#include "Process.h"
#include "MediaCaster.h"
#include "Messages.h"


Upgrade::Upgrade() {
    TRACE("Upgrade::Upgrade");    
    connProblem  = 0;
    installerUrl = configuration.getURL( configuration.getInstallerPath() );
}


void Upgrade::downloadFunction() throw (ConnectionException) {
    TRACE("Upgrade::downloadFunction");
    
    HTTPGet httpGet(installerUrl, configuration.getUser(), configuration.getPassword());
    char    bytes[1024];
    int     cnt;
    int     total = 0;
    
    setStatusMessage(hwnd, CONNECTING);    
    httpGet.connect();    
    
    string path = string(tempnam("", "installer_MediaCaster")) + ".exe";
    FILE*  fd   = fopen(path.c_str(), "wb");
    if (fd) {
        while(cnt=httpGet.read(bytes, sizeof(bytes))) {
            total += cnt;
            char status[256];
            sprintf(status, INSTALLER_DOWNLOAD, int( float(total*100./httpGet.contentLen())) );
            setStatusMessage(hwnd, status);
            fwrite(bytes, sizeof(char), cnt, fd);
        }
        
        fclose(fd);
//      Don't really need to do this since the installer closes winamp        
//      SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&plugin, ML_IPC_REMOVE_PLUGIN);
        
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
//          The installer restarts winamp in silent mode
//          SendMessage(plugin.hwndWinampParent, WM_WA_IPC, 0, IPC_RESTARTWINAMP);
        }   
    } else {
        throw ConnectionException("Cannot create temp file");
    }
}


void Upgrade::download() throw(ConnectionException) {
    TRACE("Upgrade::download");
    time_t     now   = time(NULL);
    struct tm* ctime = gmtime(&now);
    time_t     gmt   = mktime(ctime);  // This returns in GMT instead of local    
    
    if (isAvailable()) {
        downloadFunction();
        configuration.setBuildDate(gmt);
    }
}


int Upgrade::isAvailable() throw(ConnectionException) {
    TRACE("Upgrade::isAvailable");
    int status = false;
    
    try {                
//      Don't output status message since this gets called from config dialog too
//      setStatusMessage(hwnd, CONNECTING);
        HTTPInfo httpInfo(installerUrl, configuration.getUser(), configuration.getPassword());
//      setStatusMessage(hwnd, CONNECTED_UPDATING);

        LOGGER("Local ", configuration.getBuildDate());
        LOGGER("Remote", httpInfo.lastModified());
        if (httpInfo.lastModified()>configuration.getBuildDate()) {
            setStatusMessage(hwnd, UPGRADE_AVAIL_STATUS);
            status = true;
        } else {
//          setStatusMessage(hwnd, UPTODATE_STATUS);
            status = false;
        }
        connProblem = 0;
        
    } catch (HTTPAuthenticationException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        RETHROW(ex);
        
    } catch (HTTPException& ex) {
        if (ex.getErrorCode() == 404) {
            CATCH(ex);
            if (!connProblem) connectionProblemBox(hwnd, installerUrl.c_str());
            connProblem = 1;
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
    	return CONN_PROBLEM_STATUS2;        
    }
}


void Upgrade::setHwnd(HWND hwnd) {
    TRACE("Upgrade::setHwnd");
    Upgrade::hwnd = hwnd;
}
