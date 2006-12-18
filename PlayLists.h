#ifndef PLAYLISTS_H_
#define PLAYLISTS_H_

#include "DisplayList.h"


class PlayList: public DisplayListImpl {
	private:
        MasterList* origMasterList;
        
	public:
        PlayList(const char* name, const char* desc, DisplayList& displayList);
        virtual ~PlayList();
        
        virtual void setSongList(SongList*);
        virtual void abort()      const;
        virtual HWND getHwnd()    const;
        virtual void setHwnd(HWND hwnd);
        virtual int  isAborted()  const;
};


class SearchPlayList: public PlayList {
    public:
        SearchPlayList(const char*, const char*, const char*, DisplayList&);   
};


class M3uPlayList: public PlayList {
    private:
        char* m3u;
                
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

        virtual void         setHwnd(HWND hwnd)     { PlayLists::hwnd = hwnd;                 }
        virtual void         download() throw(ConnectionException);
        virtual void         clear();
        virtual int          getSize()              { return playLists->GetSize();            }
        virtual PlayList*    getPlayList(int);
        virtual PlayList*    getPlayList(const char*);
};


#endif /*PLAYLISTS_H_*/
