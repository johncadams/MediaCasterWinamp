
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


void Upgrade::downloadFunction() throw (HTTPException) {
    TRACE("Upgrade::downloadFunction");
    
    string installerUrl = configuration.getURL( configuration.getInstallerPath() );
    
    setStatusMessage(hwnd, "[Connecting...]");  
    HTTPGet httpGet(installerUrl, configuration.getUser(), configuration.getPassword());
    char    bytes[8*1024];
    int     cnt;
    int     total = 0;
    
    httpGet.connect();
    string path = string(tempnam("", "installer_MediaCaster")) + ".exe";
    FILE*  fd   = fopen(path.c_str(), "w+b");
    while(cnt=httpGet.read(bytes, sizeof(bytes))) {
        total += cnt;
        char status[32];
        sprintf(status, "[Connected] Downloading installer: %3d%%", int( float(total*100./httpGet.contentLen())) );
        setStatusMessage(hwnd, status);
        fwrite(bytes, sizeof(char), cnt, fd);
    }
    fclose(fd);
    
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&plugin, ML_IPC_REMOVE_PLUGIN);
    
    string cmdline = path +" /S /D=" +configuration.getWinampDir();
    execForegroundProcess(cmdline.c_str());
    
    remove(path.c_str());        
    SendMessage(plugin.hwndWinampParent, WM_WA_IPC, 0, IPC_RESTARTWINAMP);
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
    
    try {
        string installerUrl = configuration.getURL( configuration.getInstallerPath() );
        
//      Don't output status message since this gets called from config dialog too
//      setStatusMessage(hwnd, "[Connecting...]");
        HTTPInfo httpInfo(installerUrl, configuration.getUser(), configuration.getPassword());
//      setStatusMessage(hwnd, "[Connected] Checking for updates");

        LOGGER("Local ", configuration.getBuildDate());
        LOGGER("Remote", httpInfo.lastModified());
        if (httpInfo.lastModified()>configuration.getBuildDate()) {
            setStatusMessage(hwnd, "An upgrade is available");
            return true;
        } else {
//          setStatusMessage(hwnd, "Your installation is current");
            return false;
        }
        
    } catch (HTTPAuthenticationException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        throw ex;
        
    } catch (HTTPException& ex) {
        if (ex.getErrorCode() == 404) {
            IGNOREX(ex, "File not required");
        } else {
            throw ex;
        }
    }
    return false;
}


void Upgrade::setHwnd(HWND hwnd) {
    TRACE("Upgrade::setHwnd");
    Upgrade::hwnd = hwnd;
}
