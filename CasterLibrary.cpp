
#include "CasterLibrary.h"
#include "MediaCaster.h"
#include "TimerThread.h"
#include "HTTPGet.h"
#include "Trace.h"

/*
#define NO_THREADS
*/


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
            DisplayList* playList = playLists->getDisplayList(x);
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


void CasterLibrary::download() {
    TRACE("CasterLibrary::download");
    
#ifdef NO_THREADS
    callDownload(this);
#else
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
#endif                 
}


void CasterLibrary::downloadFunction() {
    TRACE("CasterLibrary::downloadFunction");
    
#ifdef NO_THREADS
    ::grayRefreshButton(hwnd, true);
#else
    ::showAbortButton  (hwnd, true);
#endif

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
     
#ifdef NO_THREADS
    ::grayRefreshButton(hwnd, false);
#else
    ::showAbortButton  (hwnd, false);
#endif
}


void CasterLibrary::display() {
    TRACE("CasterLibrary::display");
    currentList->display();                 
}


int CasterLibrary::checkId(int treeId) {
    TRACE("CasterLibrary::checkId");    
    if (displayList->getTreeId() == treeId) return true;
    
    for (int x=0; x<playLists->getSize(); x++) {
        DisplayList* playList = playLists->getDisplayList(x);
        if (playList->getTreeId() == treeId) return true;
    }
    
    return false;
}


int CasterLibrary::isUpgradeAvailable() {
    TRACE("CasterLibrary::isUpgradeAvailable");
    try {
        return upgrade->isAvailable();
    } catch (ConnectionException ex) {
        IGNOREX(ex, "File not required");
    }
    
    return false;
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
