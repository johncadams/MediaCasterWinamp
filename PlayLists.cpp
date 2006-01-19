
using namespace std;
#include <string>

#include "gen_ml/listview.h"

#include "MediaCaster.h"
#include "PlayLists.h"
#include "HTTPGet.h"
#include "MediaCaster.h"
#include "Trace.h"


SearchPlayList::SearchPlayList(const char* name, const char* descrip, const char* prefilter, DisplayList& rootList): 
    DisplayListImpl(name, rootList) {
    TRACE("SearchPlayList::SearchPlayList");
    SearchPlayList::prefilter = strdup(prefilter);
}


SearchPlayList::~SearchPlayList() {
    TRACE("SearchPlayList::~SearchPlayList");
    delete prefilter;
}


unsigned SearchPlayList::filterFunction(const char* filter) {
    TRACE("SearchPlayList::filterFunction");
    char newfilter[512];
    sprintf(newfilter, "\"%s\" %s", prefilter, filter);
    return DisplayListImpl::filterFunction(newfilter);
}



M3uPlayList::M3uPlayList(const char* name, const char* descrip, const char* m3u, DisplayList& displayList):
    DisplayListImpl(name, displayList) {
    TRACE("M3uPlayList::M3uPlayList");
    M3uPlayList::m3u      = strdup(m3u);
    M3uPlayList::rootList = displayList.referenceMasterList();
}


M3uPlayList::~M3uPlayList() {
    TRACE("M3uPlayList::~M3uPlayList");
    delete m3u;
}


void M3uPlayList::downloadFunction() throw(ConnectionException) {
    TRACE("M3uPlayList::downloadFunction");
    
    if (configuration.getHost()[0]=='\0') return;
    
    SongList* newSongs = new SongList();
    string    m3uUrl   = configuration.getURL(m3u);
    
    setStatusMessage(getHwnd(), "[Connecting...]");
    HTTPGet httpGet(m3uUrl, configuration.getUser(), configuration.getPassword());
    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
    httpGet.addHeader("Accept:     text/*");
    
    
    try {
        httpGet.connect();
    
        // Format:
        //   #EXTM3U
        //   #EXTINF:<length>,<display title>
        //   <path>  <-- This is all we care about
        //   ...
        
        char* buf;
        httpGet.readLine(buf);
        if (!buf || strcmp(buf,"#EXTM3U")!=0) throw ConnectionException("Improper M3U file");

        while ( !isAborted() && httpGet.readLine(buf) ) {
            if (strncmp(buf,"#EXTINF:",8)!=0) throw ConnectionException("Corrupt M3U file");
            
            httpGet.readLine(buf);
//          Song* song = masterList->getSong(buf);
Song* song = NULL;
            if (song) {
                LOGGER("Adding", song->file.c_str());
                song->addReference();
                newSongs->addSong(song);
            } else {
                LOGGER("Missing", buf);
            }
        }

    } catch (HTTPAuthenticationException& ex) {
        // Have to do it this way or this exception isn't rethrown correctly
        delete newSongs;
//      masterList->songList->clear();
        RETHROW(ex);

    } catch (ConnectionException& ex) {
        delete newSongs;
//      masterList->purge();
        RETHROW(ex);
    }

    if (isAborted()) {
        delete newSongs;
    } else {
 //     delete masterList->masterList;
 //     masterList->masterList = newSongs;
    }
    
//  LOGGER("size", masterList->getSize());
}



PlayLists::PlayLists(DisplayList& displayList): rootList(displayList) {
    TRACE("PlayLists::PlayLists");
    PlayLists::playLists  = new C_ItemList();
}


PlayLists::~PlayLists() {
    TRACE("PlayLists::~PlayLists");
    PlayLists::purge();
    delete playLists;
}


void PlayLists::purge() {
    TRACE("PlayLists::purge");
    if (playLists) {
        int i=playLists->GetSize();
        LOGGER("size",i);
        while (i>0) {
            DisplayList* displayList = getDisplayList(--i);
            displayList->deleteReference();
            playLists->Del(i);
        }
    }
}


void PlayLists::download() throw(ConnectionException) {
    TRACE("PlayLists::download");
    
    if (configuration.getHost()[0]=='\0') return;
    
    C_ItemList* newLists = new C_ItemList();
        
    string  playlistUrl = configuration.getURL( configuration.getPlaylistPath() );
    HTTPGet httpGet(playlistUrl, configuration.getUser(), configuration.getPassword());
    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
    httpGet.addHeader("Accept:     text/*");
    
    try {
        httpGet.connect();
        
        char* buf;
        int   warned = 0;
        while ( httpGet.readLine(buf) ) {
            char* title = strtok(buf,  "|");
            char* type  = strtok(NULL, "|");
            char* data  = strtok(NULL, "|");
            char* desc  = strtok(NULL, "|");            
                        
            DisplayList* list = NULL;
            if (strcmp("search",type)==0) {
                list = new SearchPlayList(title, desc, data, rootList);
                
            } else if (strcmp("m3u",type)==0) {
                list = new M3uPlayList(title, desc, data, rootList);
                list->download();
    
            } else {
                newFeatureBox(hwnd, "An unknown playlist type was found.", warned);
            }
            
            newLists->Add(list);
            delete buf;
        }
                
    } catch (HTTPAuthenticationException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        delete newLists;
        throw ex;
        
    } catch (HTTPException& ex) {
        delete newLists;
        if (ex.getErrorCode() == 404) {
            IGNOREX(ex, "File not required");
        } else {
            throw ex;
        }
    }
    
    delete playLists;
    playLists = newLists;
}
