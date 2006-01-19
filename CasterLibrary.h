#ifndef CASTERLIBRARY_H_
#define CASTERLIBRARY_H_

#include "DisplayList.h"
#include "PlayLists.h"
#include "Upgrade.h"
#include "Song.h"


class CasterLibrary {
    friend void callDownload(CasterLibrary*);
    
    private:
        int                upgradeAvail;
        HWND               hwnd;
        DisplayList*       displayList;
        PlayLists*         playLists;
        DisplayList*       currentList;
        Upgrade*           upgrade;
        
    protected:
        virtual void       downloadFunction();
        
    public:
	   CasterLibrary(int);
	   virtual ~CasterLibrary();
       
       virtual void        setTreeId(int, HWND);
       
       virtual void        clear();
       virtual void        sort();
       virtual void        download();
       virtual void        display();
       virtual void        abort();
       virtual void        enqueue() const;
       virtual void        play() const;
       virtual int         getSize();
       virtual const Song* getSong(int);
       virtual int         checkId(int);
       virtual void        drop(POINT) const;
       virtual int         isUpgradeAvailable();
       virtual void        downloadUpgrade();
};

#endif /*CASTERLIBRARY_H_*/
