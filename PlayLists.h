#ifndef PLAYLISTS_H_
#define PLAYLISTS_H_

#include "DisplayList.h"


class PlayList: public DisplayListImpl {
    public:
        PlayList(const char* name, const char* desc, DisplayList& displayList) :
            DisplayListImpl(name, displayList) {}
};


class SearchPlayList: public PlayList {
    private:
        char* prefilter;
                
    public:
        SearchPlayList(const char*, const char*, const char*, DisplayList&);
        virtual ~SearchPlayList();
        
        virtual unsigned filterFunction(const char*);
};


class M3uPlayList: public PlayList {
    private:
        char*       m3u;
        MasterList* rootList;
                
    public:
        M3uPlayList(const char*, const char*, const char*, DisplayList&);
        virtual ~M3uPlayList();
        
        virtual void downloadFunction() throw(ConnectionException);
        virtual HWND getHwnd()    const;
        virtual void setHwnd(HWND hwnd);
        virtual int  isAborted()  const;
        virtual void abort()      const;

};


class PlayLists {
    private:
        C_ItemList*  playLists;
        DisplayList& rootList;
        HWND         hwnd;
        int          connectionProblem;
        
    public:
        PlayLists(DisplayList&);        
        virtual PlayLists::~PlayLists();

        virtual void         setHwnd(HWND)          { PlayLists::hwnd = hwnd;                 }
        virtual void         download() throw(ConnectionException);
        virtual void         clear();
        virtual int          getSize()              { return playLists->GetSize();            }
        virtual PlayList*    getPlayList(int);
        virtual PlayList*    getPlayList(const char*);
};


#endif /*PLAYLISTS_H_*/
