#ifndef PLAYLISTS_H_
#define PLAYLISTS_H_

#include "DisplayList.h"


class PlayList: public DisplayListImpl {
    private:
        char* prefilter;
                
    public:
        PlayList(const char*, DisplayList&, const char* prefilter);
        virtual ~PlayList();
        
        virtual unsigned filterFunction(const char*);
};


class PlayLists {
    private:
        C_ItemList*  playLists;
        DisplayList& rootList;
        
    protected:
        virtual void purge();
        
    public:
        PlayLists(DisplayList&);        
        virtual PlayLists::~PlayLists();

        virtual void         download(HWND) throw(ConnectionException);
        virtual void         refresh (HWND) throw(ConnectionException);
        virtual void         clear();
        virtual int          getSize()              { return playLists->GetSize();             }
        virtual DisplayList* getDisplayList(int x)  { return (DisplayList*)playLists->Get(x);  }
};


#endif /*PLAYLISTS_H_*/
