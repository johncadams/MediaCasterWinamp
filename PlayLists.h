#ifndef PLAYLISTS_H_
#define PLAYLISTS_H_

#include "DisplayList.h"


class SearchPlayList: public DisplayListImpl {
    private:
        char* prefilter;
                
    public:
        SearchPlayList(const char*, const char*, const char*, DisplayList&);
        virtual ~SearchPlayList();
        
        virtual unsigned filterFunction(const char*);
};


class M3uPlayList: public DisplayListImpl {
    private:
        char*       m3u;
        MasterList* rootList;
                
    public:
        M3uPlayList(const char*, const char*, const char*, DisplayList&);
        virtual ~M3uPlayList();
        
        virtual void downloadFunction() throw(ConnectionException);
};


class PlayLists {
    private:
        C_ItemList*  playLists;
        DisplayList& rootList;
        HWND         hwnd;
        
    public:
        PlayLists(DisplayList&);        
        virtual PlayLists::~PlayLists();

        virtual void         setHwnd(HWND)          { PlayLists::hwnd = hwnd;                 }
        virtual void         download() throw(ConnectionException);
        virtual void         purge();
        virtual int          getSize()              { return playLists->GetSize();            }
        virtual DisplayList* getDisplayList(int x)  { return (DisplayList*)playLists->Get(x); }
};


#endif /*PLAYLISTS_H_*/
