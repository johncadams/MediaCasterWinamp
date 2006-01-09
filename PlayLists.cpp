
using namespace std;
#include <string>

#include "gen_ml/listview.h"

#include "MediaCaster.h"
#include "PlayLists.h"
#include "HTTPGet.h"
#include "MediaCaster.h"
#include "Trace.h"


PlayList::PlayList(const char* name, DisplayList& rootList, const char* prefilter): 
    DisplayListImpl(name, rootList) {
    TRACE("PlayList::PlayList");
    PlayList::prefilter = strdup(prefilter);
}


PlayList::~PlayList() {
    TRACE("PlayList::~PlayList");
    delete prefilter;
}


unsigned PlayList::filterFunction(const char* filter) {
    TRACE("PlayList::filterFunction");
    char newfilter[512];
    sprintf(newfilter, "\"%s\" %s", prefilter, filter);
    return DisplayListImpl::filterFunction(newfilter);
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
            delete displayList;
            playLists->Del(i);
        }
    }
}


void PlayLists::download(HWND hwnd) throw(ConnectionException) {
    TRACE("PlayLists::download");
    
    if (configuration.getHost()[0]=='\0') return;
        
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
            
                        
            DisplayList* list = NULL;
            if (strcmp("search",type)==0) {
                list = new PlayList(title, rootList, data);
    
            } else {
                newFeatureBox(hwnd, "An unknown playlist type was found.", warned);
            }
            
            if (list) playLists->Add(list);
            delete buf;
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
}


void PlayLists::refresh(HWND hwnd) throw(ConnectionException) {
    TRACE("PlayLists::refresh");
    PlayLists::clear();
    PlayLists::download(hwnd);
}


void PlayLists::clear() {
    TRACE("PlayLists::clear");
    PlayLists::purge();
}
