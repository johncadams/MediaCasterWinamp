
using namespace std;
#include <string>

#include "gen_ml/listview.h"

#include "MediaCaster.h"
#include "PlayLists.h"
#include "HTTPGet.h"
#include "MediaCaster.h"
#include "Trace.h"
#include "Messages.h"


SearchPlayList::SearchPlayList(const char* name, const char* desc, const char* prefilter, DisplayList& displayList): 
    PlayList(name, desc, displayList) {
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
    return PlayList::filterFunction(newfilter);
}



M3uPlayList::M3uPlayList(const char* name, const char* desc, const char* m3u, DisplayList& displayList):
    PlayList(name, desc, displayList) {
    TRACE("M3uPlayList::M3uPlayList");
    M3uPlayList::m3u      = strdup(m3u);
    M3uPlayList::rootList = masterList;
    PlayList::masterList  = new MasterList(name);
}


M3uPlayList::~M3uPlayList() {
    TRACE("M3uPlayList::~M3uPlayList");
    delete masterList; // We should be the only one
    delete m3u;
    masterList = rootList;
}


void M3uPlayList::downloadFunction() throw(ConnectionException) {
    TRACE("M3uPlayList::downloadFunction");
    
    if (configuration.getHost()[0]=='\0') return;
    
    SongList* newSongs = new SongList();
    string    m3uUrl   = configuration.getURL(m3u);
    
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
        delete buf;
        
        while ( !isAborted() && httpGet.readLine(buf) ) {            
            if (strncmp(buf,"#EXTINF:",8)!=0) throw ConnectionException("Improper M3U file");
            delete buf;
            
            httpGet.readLine(buf);
            if (!buf) throw ConnectionException("Improper M3U file");
            
            Song* song = rootList->getSong(buf);
            if (song) {
                LOGGER("Adding", song->file.c_str());
                song->addReference();
                newSongs->addSong(song);
            } else {
                LOGGER("Missing", buf);
            }
                        
            delete buf;
        }

    } catch (HTTPAuthenticationException& ex) {
        // Have to do it this way or this exception isn't rethrown correctly
        delete newSongs;
        masterList->clear();
        RETHROW(ex);

    } catch (ConnectionException& ex) {
        delete newSongs;
        masterList->clear();
        RETHROW(ex);
    }

    if (isAborted()) {
        delete newSongs;
    } else {
        masterList->songList->purge();
        delete masterList->songList;
        masterList->songList = newSongs;
    }
    
    LOGGER("size", masterList->getSize());
}


void M3uPlayList::abort() const {
    TRACE("M3uPlayList::abort");
    rootList->abort();
}


int M3uPlayList::isAborted() const {
//  TRACE("M3uPlayList::abort");
    return rootList->isAborted();
}


HWND M3uPlayList::getHwnd() const {
//  TRACE("M3uPlayList::getHwnd");
    return rootList->getHwnd();
}


void M3uPlayList::setHwnd(HWND hwnd) {
    TRACE("M3uPlayList::setHwnd");
    rootList->setHwnd(hwnd);
}



PlayLists::PlayLists(DisplayList& displayList): rootList(displayList) {
    TRACE("PlayLists::PlayLists");
    PlayLists::playLists  = new C_ItemList();
}


PlayLists::~PlayLists() {
    TRACE("PlayLists::~PlayLists");
    PlayLists::clear();
    delete playLists;
}


void PlayLists::clear() {
    TRACE("PlayLists::clear");
    if (playLists) {
        int i=playLists->GetSize();
        LOGGER("size",i);
        while (i>0) {
            PlayList* playList = getPlayList(--i);
            playList->deleteReference();
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
                 
            PlayList* playList = getPlayList(title);
            if (strcmp("search",type)==0) {
                if (!playList) playList = new SearchPlayList(title, desc, data, rootList);
                else           playList->addReference();
                
            } else if (strcmp("m3u",type)==0) {
                if (!playList) playList = new M3uPlayList(title, desc, data, rootList);
                else           playList->addReference();
                playList->download();
                
            } else if (type[0]=='#') {
                // Commented out, ignored
                
            } else {
                newFeatureBox(hwnd, UNKNOWN_PLAYLIST_TYPE, warned);    
                playList = NULL;
            }
            
            if (playList) newLists->Add(playList);                
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
    
    clear();
    delete playLists;
    playLists = newLists;
}


PlayList* PlayLists::getPlayList(int ndx) {
    TRACE("PlayLists::getPlayList");
    
    return (PlayList*)playLists->Get(ndx);
}


PlayList* PlayLists::getPlayList(const char* name) {
    TRACE("PlayLists::getPlayList");
    
    for (int i=0; i<playLists->GetSize(); i++) {
        PlayList* playList = (PlayList*)playLists->Get(i);
        if (strcmp(playList->getName(), name)==0) {
            return playList;
        }
    }
    return NULL;
}
