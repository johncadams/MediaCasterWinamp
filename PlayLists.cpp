#include <string>

#include "gen_ml/listview.h"

#include "MediaCaster.h"
#include "PlayLists.h"
#include "HTTPGet.h"
#include "MediaCaster.h"
#include "Trace.h"
#include "Messages.h"

using namespace std;


PlayList::PlayList(const char* name, const char* desc, DisplayList& displayList) :
	DisplayListImpl(name, displayList), origMasterList(NULL) { 
	TRACE("PlayList::PlayList");
}


PlayList::~PlayList() {
    TRACE("PlayList::~PlayList");    
    if (origMasterList != NULL) {
    	masterList->deleteReference();
    	masterList = origMasterList;
    	origMasterList = NULL;
    }
}


void PlayList::setSongList(SongList* songList) {
	TRACE("PlayList::setSongLost");
	if (origMasterList == NULL) {
		origMasterList = masterList;
	} else {
		// We're being asked to re-associate a songList, probably shouldn't do this
		delete masterList->songList;
    	masterList->deleteReference();
	}
	
    masterList = new MasterList(name);    
	masterList->songList = songList;
}


void PlayList::abort() const {
    TRACE("PlayList::abort");
    if (origMasterList) origMasterList->abort();
    else                DisplayListImpl::abort();
}


int PlayList::isAborted() const {
//  TRACE("PlayList::abort");
    if (origMasterList) return origMasterList->isAborted();
    else                return DisplayListImpl::isAborted();
}


HWND PlayList::getHwnd() const {
//  TRACE("PlayList::getHwnd");
    if (origMasterList) return origMasterList->getHwnd();
    else                return DisplayListImpl::getHwnd();
}


void PlayList::setHwnd(HWND hwnd) {
    TRACE("PlayList::setHwnd");
    if (origMasterList) origMasterList->setHwnd(hwnd);
    else                DisplayListImpl::setHwnd(hwnd);
}

            
            
SearchPlayList::SearchPlayList(const char* name, const char* desc, const char* search, DisplayList& displayList): 
    PlayList(name, desc, displayList) {
    TRACE("SearchPlayList::SearchPlayList");    
    
    SongList* newSongs = new SongList();
    
    filterFunction(search);
    LOGGER("Search", search);
    for (int i=0; i<getSongList()->getSize(); i++) {
		const char* mp3  = getSong(i)->file.c_str();
		Song*       song = masterList->getSong(mp3);
		if (song) {
            LOGGER("Adding", mp3);
            song->addReference();
            newSongs->addSong(song);
        } else {
            LOGGER("Missing", mp3);
        }
	}
	
    setSongList(newSongs);
}



M3uPlayList::M3uPlayList(const char* name, const char* desc, const char* m3u, DisplayList& displayList):
    PlayList(name, desc, displayList) {
    TRACE("M3uPlayList::M3uPlayList");
    M3uPlayList::m3u = strdup(m3u);
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
        if (!buf || strcmp(buf,"#EXTM3U")!=0) throw(ConnectionException("Improper M3U file"));
        delete buf;
        
        while ( !isAborted() && httpGet.readLine(buf) ) {            
            if (strncmp(buf,"#EXTINF:",8)!=0) throw(ConnectionException("Improper M3U file"));
            delete buf;
            
            httpGet.readLine(buf);
            if (!buf) throw ConnectionException("Improper M3U file");
            
            Song* song = masterList->getSong(buf);
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
        CATCH(ex);
        delete newSongs;
        RETHROW(ex);

    } catch (ConnectionException& ex) {
    	CATCH(ex);
        delete newSongs;
        RETHROW(ex);
    }

    if (isAborted()) {
        delete newSongs;
    } else {
    	setSongList(newSongs);
    }
}



PlayLists::PlayLists(DisplayList& displayList) : rootList(displayList)  {
    TRACE("PlayLists::PlayLists");
    PlayLists::playLists = new C_ItemList();
    displayList.addReference();
}


PlayLists::~PlayLists() {
    TRACE("PlayLists::~PlayLists");
    rootList.deleteReference();
    PlayLists::clear();
    delete playLists;
}


static C_ItemList* downloadFunction(DisplayList& rootList) throw(ConnectionException) {
    TRACE("downloadFunction");
    
    if (configuration.getHost()[0]=='\0') return NULL;
    
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
            
            const char* title = strtok(buf,  "|");
            const char* type  = strtok(NULL, "|");
            const char* data  = strtok(NULL, "|");
            const char* desc  = strtok(NULL, "|");            
                 
            PlayList* playList = NULL;
            if (strcmp("search",type)==0 || strcmp("search2",type)==0) {
                playList = new SearchPlayList(title, desc, data, rootList);
                
            } else if (strcmp("m3u",type)==0) {
                playList = new M3uPlayList(title, desc, data, rootList);                
                playList->download();
                
            } else if (type[0]=='#') {
                // Commented out, ignored
                
            } else {
                newFeatureBox(rootList.getHwnd(), UNKNOWN_PLAYLIST_TYPE, warned);    
            }
            
            if (playList) {
            	LOGGER("Adding playlist", title);
            	newLists->Add(playList);                
            }
            delete buf;
        }
                
    } catch (HTTPAuthenticationException& ex) {
        // If we don't catch this this way it gets reported as HTTPException
        CATCH(ex);
        delete newLists;
        RETHROW(ex);
        
    } catch (HTTPException& ex) {
    	CATCH(ex);
        delete newLists;
        if (ex.getErrorCode() == 404) {
            IGNOREX(ex, "File not required");
        } else {
            RETHROW(ex);
        }
    }
    
    return newLists;
}

void PlayLists::download() throw(ConnectionException) {
	TRACE("PlayLists::download");
	C_ItemList* newLists = NULL;
	try {
		newLists = ::downloadFunction(rootList);
		clear();
    	delete playLists;
    	playLists = newLists;
    	
	} catch (HTTPAuthenticationException& ex) {
		CATCH(ex);
		if (newLists) delete newLists;
        RETHROW(ex);    
    } catch (HTTPException& ex) {
    	CATCH(ex);
        if (newLists) delete newLists;
        RETHROW(ex);
    }
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


void PlayLists::clear() {
    TRACE("PlayLists::clear");
    if (playLists) {
        int i=playLists->GetSize();
        LOGGER("size",i);
        while (i>0) {      	
            PlayList* playList = getPlayList(--i);            
            playLists->Del(i);
            playList->deleteReference();
        }
    }
}
