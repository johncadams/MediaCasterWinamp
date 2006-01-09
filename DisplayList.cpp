#include "gen_ml/listview.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "DisplayList.h"
#include "HTTPGet.h"
#include "Process.h"
#include "Trace.h"


static int stopLoading;


static void parseQuickSearch(char* out, const char* in) {
    TRACE("parseQuickSearch");
    int inquotes = 0;
    int neednull = 0;
    
    while (*in) {
        char c = *in++;
        if (c != ' ' && c != '\t' && c != '\"') {
            neednull = 1;
            *out++   = c;
      
        } else if (c == '\"') {
            inquotes=!inquotes;
            if (!inquotes) {
                *out++   = 0;
                neednull = 0;
            }
      
        } else {
            if      (inquotes) *out++=c;
            else if (neednull) {
                *out++   = 0;
                neednull = 0;
            }
        }
    }
    *out++ = 0;
    *out++ = 0;
}


static int in_string(const char* string, const char* substring) {
//  TRACE("in_string");  This gets called to many times
    if (!string)     return 0;
    if (!*substring) return 1;
    int l=strlen(substring);
    while (string[0]) if (!strnicmp(string++,substring,l)) return 1; 
    return 0;
}



MasterList::MasterList(const char* name) {
    TRACE("MasterList::MasterList");
    MasterList::name           = strdup(name);
    MasterList::masterList     = new SongList();
    MasterList::refCount       = 1;
    JNL::open_socketlib();
}


MasterList::~MasterList() {
    TRACE("MasterList::~MasterList");
    delete masterList;
    delete name;
    JNL::close_socketlib();
}


MasterList* MasterList::addReference() {
    TRACE("MasterList::addReference");
    this->refCount++;
    return this;
}


void MasterList::deleteReference() {
    TRACE("MasterList::deleteReference");
    if (--refCount==0) {
        LOGGER("refCount",refCount);
        delete this;
    }
}


void MasterList::setHwnd(HWND hwnd) {
    TRACE("MasterList::setHwnd");
    MasterList::hwnd = hwnd;
}


void MasterList::clear() {
    TRACE("MasterList::clear");
    masterList->purge();
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
    
    setStatusMessage(hwnd, "[Connecting...]");
    HTTPGet httpGet(masterListUrl, configuration.getUser(), configuration.getPassword());
    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
    httpGet.addHeader("Accept:     text/*");
    
    try {           
        httpGet.connect();
        char* buf;
        int   total = 0;
        int   cnt;

        while ( !stopLoading && (cnt=httpGet.readLine(buf)) > 0) {
            char* title   = strtok(buf,  "|");
            char* artist  = strtok(NULL, "|");
            char* album   = strtok(NULL, "|");
            char* genre   = strtok(NULL, "|");
            char* year    = strtok(NULL, "|");
            char* track   = strtok(NULL, "|");
            char* len     = strtok(NULL, "|");
            char* file    = strtok(NULL, "|");
            char* comment = strtok(NULL, "|");

            newSongs->addSong( new Song(artist, title, album, genre, file, len, track, year, comment) );
            
            char status[64];
            total += cnt+1;
            sprintf(status, "[Connected] Retrieving list: %3d%%", int( float(total*100./httpGet.contentLen())) );
            setStatusMessage(hwnd, status);
            delete buf;
        }

    } catch (HTTPAuthenticationException& ex) {
        // Have to do it this way or this exception isn't rethrown correctly
        delete newSongs;
        masterList->purge();
        RETHROW(ex);

    } catch (ConnectionException& ex) {
        delete newSongs;
        masterList->purge();
        RETHROW(ex);
    }

    if (stopLoading) {
        delete newSongs;
    } else {
        delete masterList;
        masterList = newSongs;
    }
    
    LOGGER("size", masterList->getSize());
}




DisplayListImpl::DisplayListImpl(int treeId) {
    TRACE("DisplayListImpl::DisplayListImpl");
    DisplayListImpl::name        = strdup(PLUGIN_NAME);
    DisplayListImpl::masterList  = new MasterList(name);
    DisplayListImpl::displayList = new SongList();
    DisplayListImpl::treeId      = treeId;   
}


DisplayListImpl::DisplayListImpl(const char* name, DisplayList& clone) {
    TRACE("DisplayListImpl::DisplayListImpl (clone)");
    DisplayListImpl::name        = strdup(name);
    DisplayListImpl::masterList  = clone.getMasterList();
    DisplayListImpl::displayList = new SongList();
                
    // add the item to the tree
    mlAddTreeItemStruct mla = {clone.getTreeId(),(char*)name,1,};
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&mla, ML_IPC_ADDTREEITEM);
    treeId = mla.this_id;
}


DisplayListImpl::~DisplayListImpl() {
    TRACE("DisplayListImpl::~DisplayListImpl");
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, treeId, ML_IPC_DELTREEITEM);
    delete displayList;
    masterList->deleteReference();
    delete name;
}


MasterList* DisplayListImpl::getMasterList() {
    TRACE("DisplayListImpl::getMasterList");
    return masterList->addReference();
}


const char* DisplayListImpl::getName() const {
    TRACE("DisplayListImpl::getName");
    return DisplayListImpl::name;
}


int DisplayListImpl::getSize() const {
//  TRACE("DisplayListImpl::getSize");
    return displayList->getSize();
}


const Song* DisplayListImpl::getSong(int i) const {
//  TRACE("DisplayListImpl::getSong");
    return displayList->getSong(i);
}


int DisplayListImpl::getTreeId() const {
    TRACE("DisplayListImpl::getTreeId");
    return DisplayListImpl::treeId;
}


void DisplayListImpl::setHwnd(HWND hwnd) {
    TRACE("DisplayListImpl::setHwnd");
    LOGGER("hwnd",(int)hwnd);
    DisplayListImpl::hwnd = hwnd;
    masterList->setHwnd(hwnd);
}


void DisplayListImpl::download() throw(ConnectionException) {
    TRACE("DisplayListImpl::download");
    downloadFunction();
}


void DisplayListImpl::abort() const {
    TRACE("DisplayListImpl::abort");
    stopLoading = 1;
}


void DisplayListImpl::clear() {
    TRACE("DisplayListImpl::clear");
    displayList->purge();
    masterList->clear();
}


void DisplayListImpl::sortFunction() {
    TRACE("DisplayListImpl::sortFunction");
    displayList->sort();
}


void DisplayListImpl::sort() {
    TRACE("DisplayListImpl::sort");
    sortFunction();
    ListView_SetItemCount(listView.getwnd(),0);
    ListView_SetItemCount(listView.getwnd(),displayList->getSize());
    ListView_RedrawItems (listView.getwnd(),0,displayList->getSize()-1);
}


unsigned DisplayListImpl::filterFunction(const char* filter) {
    TRACE("DisplayListImpl::filterFunction");
    char filteritems[300];

    ::parseQuickSearch(filteritems, filter);

    unsigned length = 0;
    displayList->purge();
    for (int i=0; i<masterList->getSize(); i++) {        
        Song* song = masterList->getSong(i);
        char year[32]="";
        if (song->year < 5000 && song->year > 0) sprintf(year,"%d",song->year);
        
        char* p = filteritems;
        if (*p) {
            while (*p) {
                // search for 'p' in the song
                if (!::in_string(song->album  .c_str(),p) && 
                    !::in_string(song->artist .c_str(),p) &&
                    !::in_string(song->title  .c_str(),p) &&
                    !::in_string(song->genre  .c_str(),p) &&
                    !::in_string(song->comment.c_str(),p) &&
                    !::in_string(year,p))            break;

                p+=strlen(p)+1;
            }
            if (*p) continue;
        }
        length += song->songlen;
        displayList->addSong( song->addReference() );
    }
    return length;
}


void DisplayListImpl::filter() {
    TRACE("DisplayListImpl::filter");
    char filter[256];
    unsigned filterlen = 0;  
    
    ::getSearchString(hwnd, filter, sizeof(filter));
    filterlen = filterFunction(filter);
    
    char status[512];
    int  days  = filterlen/86400;
    int  count = filterlen%86400;
    int  hours = count/3600;
    int  mins  =(count/60)%60;
    int  secs  = count%60;
    if (days==0) wsprintf(status,"%d items [%d:%02d:%02d]",         displayList->getSize(),     hours,mins,secs);
    else         wsprintf(status,"%d items [%d days+%d:%02d:%02d]", displayList->getSize(),days,hours,mins,secs);
    ::setStatusMessage(hwnd, status);
}


void DisplayListImpl::play() const {
    TRACE("DisplayListImpl::play");
    displayList->play();
}


void DisplayListImpl::enqueue() const {
    TRACE("DisplayListImpl::enqueue");
    displayList->enqueue();
}

        
void DisplayListImpl::drop(POINT p) const {
    TRACE("DisplayListImpl::drop");
    mlDropItemStruct m={ML_TYPE_ITEMRECORDLIST,NULL,0};
    m.p=p;
    m.flags=ML_HANDLEDRAG_FLAG_NOCURSOR;
    SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDRAG);

    if (m.result>0) {
        itemRecordList recList={0,}; 
        displayList->toItemRecordList(recList, 1);
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
    DisplayListImpl::search();
    ListView_SetItemCount(listView.getwnd(), displayList->getSize());    
}


void DisplayListImpl::search() {
    TRACE("DisplayListImpl::search");
    filter();
    sort();    
}
