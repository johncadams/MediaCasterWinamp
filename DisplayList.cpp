#include "gen_ml/listview.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "DisplayList.h"
#include "Http.h"
#include "Process.h"
#include "Trace.h"
#include "Messages.h"
#include "Search.h"


MasterList::MasterList(const char* name, SongList* songList) {
    TRACE("MasterList::MasterList");
    MasterList::name           = strdup(name);
    MasterList::songList       = songList;
    MasterList::refCount       = 1;
    JNL::open_socketlib();
}


MasterList::~MasterList() {
    TRACE("MasterList::~MasterList");
    delete songList;
    delete name;
    JNL::close_socketlib();
}


MasterList* MasterList::addReference() {
    TRACE("MasterList::addReference");
    this->refCount++;
    LOGGER("refCount",refCount);
    return this;
}


void MasterList::deleteReference() {
    TRACE("MasterList::deleteReference");
    refCount--;
    LOGGER("refCount",refCount);
    if (refCount==0) delete this;
}


void MasterList::setHwnd(HWND hwnd) {
    TRACE("MasterList::setHwnd");
    MasterList::hwnd = hwnd;
}


void MasterList::clear() {
    TRACE("MasterList::clear");
    songList->clear();
}


void MasterList::download() throw(ConnectionException) {
    TRACE("MasterList::download");
    downloadFunction();
}


void MasterList::downloadFunction() throw(ConnectionException)  {
    TRACE("MasterList::downloadFunction");
    if (configuration.getHost()[0]=='\0') return;
        
    stopLoading = 0;
    
    SongList* newSongs      = new SongList();
    string    masterListUrl = configuration.getURL( configuration.getLibraryPath() );

    setStatusMessage(hwnd, CONNECTING);
    HTTPGet httpGet(masterListUrl, configuration.getUser(), configuration.getPassword());
    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
    httpGet.addHeader("Accept:     text/*");
    
    try {           
        httpGet.connect();
        char* buf;
        int   total = 0;

        while ( !stopLoading && httpGet.readLine(buf)) {
        	int   cnt = strlen(buf);
        	int   ndx = 0;
            char* fld[9];
            fld[0] = buf;
            for (char* p=buf; *p; p++) {
            	if (*p=='|') {
            		*p = 0;
            		fld[++ndx] = p+1;
            	}
            }
            
			char* title   = fld[0];
            char* artist  = fld[1];
            char* album   = fld[2];
            char* genre   = fld[3];
            char* year    = fld[4];
            char* track   = fld[5];
            char* len     = fld[6];
            char* file    = fld[7];
            char* comment = fld[8];
            newSongs->addSong( new Song(artist, title, album, genre, file, len, track, year, comment) );
            
            char status[256];
            total += cnt+1;
            sprintf(status, DISPLIST_DOWNLOAD, int( float(total*100./httpGet.contentLen())) );
            setStatusMessage(hwnd, status);
            delete buf;
        }

    } catch (HTTPAuthenticationException& ex) {
        // Have to do it this way or this exception isn't rethrown correctly
        CATCH(ex);
        delete newSongs;
        songList->clear();
        RETHROW(ex);

    } catch (ConnectionException& ex) {
    	CATCH(ex);
        delete newSongs;
        songList->clear();
        RETHROW(ex);
    }

    if (stopLoading) {
        delete newSongs;
    } else {
        delete songList;
        songList = newSongs;
    }
    
    LOGGER("size", songList->getSize());
}




DisplayListImpl::DisplayListImpl(int treeId) {
    TRACE("DisplayListImpl::DisplayListImpl");
    DisplayListImpl::name        = strdup("ROOT");
    DisplayListImpl::masterList  = new MasterList(name, new SongList());
    DisplayListImpl::songList    = new SongList();
    DisplayListImpl::treeId      = treeId;
    DisplayListImpl::parentId    = 0;
    DisplayListImpl::refCount    = 1;
}


DisplayListImpl::DisplayListImpl(const char* name, DisplayList& clone) {
    TRACE("DisplayListImpl::DisplayListImpl (clone)");
    DisplayListImpl::name        = strdup(name);
    DisplayListImpl::masterList  = clone.referenceMasterList();
    DisplayListImpl::songList    = new SongList();
    DisplayListImpl::refCount    = 1;
                
    // add the item to the tree
    mlAddTreeItemStruct mla = {clone.getTreeId(),(char*)name,1,};
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&mla, ML_IPC_ADDTREEITEM);
    treeId   = mla.this_id;
    parentId = clone.getTreeId();
}


DisplayListImpl::~DisplayListImpl() {
    TRACE("DisplayListImpl::~DisplayListImpl");
    
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, treeId, ML_IPC_DELTREEITEM);
    
    delete songList;
    masterList->deleteReference();
    delete name;
}


MasterList* DisplayListImpl::referenceMasterList() {
    TRACE("DisplayListImpl::referenceMasterList");
    return masterList->addReference();
}


DisplayList* DisplayListImpl::addReference() {
    TRACE("DisplayListImpl::addReference");
    this->refCount++;
	LOGGER("refCount",refCount);
    return this;
}


void DisplayListImpl::deleteReference() {
    TRACE("DisplayListImpl::deleteReference");
    refCount--;
    LOGGER("refCount",refCount);
    if (refCount==0) delete this;
}


const char* DisplayListImpl::getName() const {
    TRACE("DisplayListImpl::getName");
    return DisplayListImpl::name;
}


const SongList* DisplayListImpl::getSongList() const {
//  TRACE("DisplayListImpl::getSongList");
    return songList;
}


int DisplayListImpl::getSize() const {
//  TRACE("DisplayListImpl::getSize");
    return songList->getSize();
}


const Song* DisplayListImpl::getSong(int i) const {
//  TRACE("DisplayListImpl::getSong");
    return songList->getSong(i);
}


int DisplayListImpl::getTreeId() const {
    TRACE("DisplayListImpl::getTreeId");
    return DisplayListImpl::treeId;
}


void DisplayListImpl::download() throw(ConnectionException) {
    TRACE("DisplayListImpl::download");
    downloadFunction();
}


void DisplayListImpl::abort() const {
    TRACE("DisplayListImpl::abort");
    masterList->abort();
}


int DisplayListImpl::isAborted() const {
//  TRACE("DisplayListImpl::abort");
    return masterList->isAborted();
}


void DisplayListImpl::clear() {
    TRACE("DisplayListImpl::clear");
    songList->clear();
    masterList->clear();
}


HWND DisplayListImpl::getHwnd() const {
//  TRACE("DisplayListImpl::getHwnd");
    return masterList->getHwnd();
}


void DisplayListImpl::setHwnd(HWND hwnd) {
    TRACE("DisplayListImpl::setHwnd");
    masterList->setHwnd(hwnd);
}


void DisplayListImpl::sortFunction() {
    TRACE("DisplayListImpl::sortFunction");
    songList->sort();
}


void DisplayListImpl::sort() {
    TRACE("DisplayListImpl::sort");
    sortFunction();
    ListView_SetItemCount(listView.getwnd(),0);
    ListView_SetItemCount(listView.getwnd(),songList->getSize());
    ListView_RedrawItems (listView.getwnd(),0,songList->getSize()-1);
}


unsigned DisplayListImpl::filterFunction(const char* filter) {
    TRACE("DisplayListImpl::filterFunction");
    return doSearch(masterList, songList, filter);
}


void DisplayListImpl::filter() {
    TRACE("DisplayListImpl::filter");
    char filter[256];
    unsigned filterlen = 0;  
    
    ::getSearchString(getHwnd(), filter, sizeof(filter));
    filterlen = filterFunction(filter);
    
    char status[256];
    int  days  = filterlen/86400;
    int  count = filterlen%86400;
    int  hours = count/3600;
    int  mins  =(count/60)%60;
    int  secs  = count%60;
    if (days==0) wsprintf(status, NUMITEMS_NODAYS, songList->getSize(),     hours,mins,secs);
    else         wsprintf(status, NUMITEMS_WDAYS,  songList->getSize(),days,hours,mins,secs);
    ::setStatusMessage(getHwnd(), status);
}


void DisplayListImpl::play() const {
    TRACE("DisplayListImpl::play");
    songList->play();
}


void DisplayListImpl::enqueue() const {
    TRACE("DisplayListImpl::enqueue");
    songList->enqueue();
}

        
void DisplayListImpl::drop(POINT p) const {
    TRACE("DisplayListImpl::drop");
    mlDropItemStruct m={ML_TYPE_ITEMRECORDLIST,NULL,0};
    m.p=p;
    m.flags=ML_HANDLEDRAG_FLAG_NOCURSOR;
    SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDRAG);

    if (m.result>0) {
        itemRecordList recList={0,}; 
        songList->toItemRecordList(recList, 1);
        if (recList.Size) {
            m.flags  = 0;
            m.result = 0;
            m.data   = &recList;                 
            SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDROP);
        }
        delete recList.Items;
    }
}


void DisplayListImpl::downloadFunction() throw(ConnectionException) {
    TRACE("DisplayListImpl::downloadFunction");
    masterList->download();
    DisplayListImpl::display();
    ListView_SetItemCount(listView.getwnd(), songList->getSize());    
}


void DisplayListImpl::display() {
    TRACE("DisplayListImpl::display");
    filter();
    sort();    
}
