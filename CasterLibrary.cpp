#include <sys/types.h>
#include <dirent.h>
     
#include "CasterLibrary.h"
#include "MediaCaster.h"
#include "TimerThread.h"
#include "Http.h"
#include "Trace.h"


static TimerThread* downloadThread;

void callDownload(CasterLibrary* library) {
    TRACE("callDownload");
    library->downloadFunction();
}


CasterLibrary::CasterLibrary(int treeId) {
    TRACE("CasterLibrary::CasterLibrary");
    displayList = new DisplayListImpl(treeId);                
    playLists   = new PlayLists(*displayList);
    upgrade     = new Upgrade();
}


CasterLibrary::~CasterLibrary() {
    TRACE("CasterLibrary::~CasterLibrary");
    delete upgrade;
    delete playLists;
    displayList->deleteReference();
}


void CasterLibrary::setTreeId(int treeId, HWND hwnd) {
    TRACE("CasterLibrary::setTreeId");
    
    currentList = displayList;
    if (playLists) {
        playLists->setHwnd(hwnd);
        for (int x=0; x<playLists->getSize(); x++) {
            PlayList* playList = playLists->getPlayList(x);
            if (playList->getTreeId() == treeId) {
                currentList = playList;
                break;
            }
        }
    }
    
    if (currentList) {
        CasterLibrary::hwnd = hwnd;
        currentList->setHwnd(hwnd);
        configuration.setPlaylist(currentList->getName());
    }
    
    upgrade->setHwnd(hwnd);
}
 
      
void CasterLibrary::clear() {
    TRACE("CasterLibrary::clear");
    currentList->clear();
}


void CasterLibrary::sort() {
    TRACE("CasterLibrary::sort");
    currentList->sort();
}


void CasterLibrary::abort() {
    TRACE("CasterLibrary::abort");
    currentList->abort();
    if (downloadThread) downloadThread->stop();
}


void CasterLibrary::play() const {
    TRACE("CasterLibrary::play");
    currentList->play();
}


void CasterLibrary::enqueue() const {
    TRACE("CasterLibrary::enqueue");
    currentList->enqueue();
}


void CasterLibrary::save() const {
    TRACE("CasterLibrary::save");
    currentList->save();
}


int CasterLibrary::getSize() {
//  TRACE("CasterLibrary::getSize");
    return currentList->getSize();
}


const Song* CasterLibrary::getSong(int index) {
//  TRACE("CasterLibrary::getSong");
    return currentList->getSong(index);
}


void CasterLibrary::drop(POINT point) const {
    TRACE("CasterLibrary::drop");
    currentList->drop(point);                  
}


void CasterLibrary::clearCache() {
	TRACE("CasterLibrary::clearCache");
	string  dir  = configuration.getCacheDir();
	DIR*    dirp = opendir(dir.c_str());
	
	if (dirp) {
		for (dirent* dp=readdir(dirp); dp; dp=readdir(dirp)) {
			string file = dp->d_name;
			if (file[0]!='.') {
				string path = dir+file;
				LOGGER("Removing", path.c_str());
				if (remove(path.c_str())!=0) {
					LOGGER("ERROR", strerror(errno));
				}
			}
		}
	}
	closedir(dirp);
}


void CasterLibrary::download() {
    TRACE("CasterLibrary::download");
    
	if (!configuration.isThreaded()) {
    	callDownload(this);
    	return;
    }
    	
    if (downloadThread == NULL) {
        downloadThread = new TimerThread(0, (Procedure)::callDownload, this);
    }
    if (downloadThread->isRunning()) {
        // We are in the thread so don't try to start another
        LOGGER("THREAD", "reusing");
        callDownload(this);
    } else {
        LOGGER("THREAD", "spawning");
        downloadThread->start();
    }        
}


void CasterLibrary::downloadFunction() {
    TRACE("CasterLibrary::downloadFunction");
    
	if (configuration.isThreaded()) ::showAbortButton  (hwnd, true);
    else                            ::grayRefreshButton(hwnd, true);

    ListView_SetItemCount(listView.getwnd(),0);     
    try {
        if (configuration.isAutoUpdate()) upgrade->download();
        displayList->download();
        playLists->download();
        ::setConnectionSuccess();

    } catch (HTTPAuthenticationException& ex) {
        CATCH(ex);
        ::setConnectionFailed();        
        ::authDialog(hwnd);
        
    } catch (ConnectionException& ex) {
        CATCH(ex);
        ::setConnectionFailed();
        ::connectionProblemBox(hwnd, ex.getError());        
    }

    if (configuration.isThreaded()) ::showAbortButton  (hwnd, false);
    else                            ::grayRefreshButton(hwnd, false);
}


void CasterLibrary::display() {
    TRACE("CasterLibrary::display");
    currentList->display();                 
}


int CasterLibrary::checkId(int treeId) {
    TRACE("CasterLibrary::checkId");    
    if (displayList->getTreeId() == treeId) return true;
    for (int x=0; x<playLists->getSize(); x++) {
        PlayList* playList = playLists->getPlayList(x);
        if (playList->getTreeId() == treeId) return true;
    }
    
    return false;
}


int CasterLibrary::isUpgradeAvailable() {
    TRACE("CasterLibrary::isUpgradeAvailable");
    try {
        return upgrade->isAvailable();
    } catch (ConnectionException& ex) {
        IGNOREX(ex, "File not required");
    }
    
    return false;
}


const char* CasterLibrary::getUpgradeAvailableStatus() {
    TRACE("CasterLibrary::getUpgradeAvailableStatus");
    return upgrade->getIsAvailableStatus();
}


void CasterLibrary::downloadUpgrade() {
    TRACE("CasterLibrary::downloadUpgrade");
    try {
        upgrade->download();
    }
    catch (ConnectionException& ex) {
        CATCH(ex);
        ::setConnectionFailed();
        ::connectionProblemBox(hwnd, ex.getError());
    }
}
