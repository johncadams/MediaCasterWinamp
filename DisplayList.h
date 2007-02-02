#ifndef DISPLAYLIST_H
#define DISPLAYLIST_H

#include "Http.h"
#include "Song.h"
#include "SongList.h"

class M3uPlayList;

class MasterList {
    private:
        char*        name;
        SongList*    songList;
        HWND         hwnd;
        int          refCount;
        int          stopLoading;
                
    protected:
   		virtual     ~MasterList();
        void         downloadFunction() throw(ConnectionException);

    public:
        MasterList(const char*, SongList*);
        MasterList*  addReference();
        
        void         deleteReference();
        
        void         clear();
        void         download() throw(ConnectionException);
        void         setHwnd(HWND);
        void         abort()                        { stopLoading = 1;                      }
        int          isAborted()              const { return stopLoading;                   }

        HWND         getHwnd()                const { return hwnd;                          }
        int          getSize()                const { return songList->getSize();           }
        Song*        getSong(int ndx)         const { return songList->getSong(ndx);        }
        Song*        getSong(const char* pth) const { return songList->getSong(pth);        }
};


class DisplayList {
    protected:
       virtual ~DisplayList() {}
       
    public:                
        virtual const char*  getName()            const = 0;
        virtual int          getSize()            const = 0;
        virtual HWND         getHwnd()            const = 0;
        virtual int          getTreeId()          const = 0;
        virtual const Song*  getSong(int ndx)     const = 0;
        virtual int          isAborted()          const = 0;
        virtual void         clear()                    = 0;
        virtual void         display()                  = 0;
        virtual void         sort()                     = 0;
        virtual void         filter()                   = 0;
        virtual void         setHwnd(HWND hwnd)         = 0;
        virtual void         play()               const = 0;
        virtual void         enqueue()            const = 0;
        virtual void         save()               const = 0;
        virtual void         drop(POINT)          const = 0;
        virtual void         abort()              const = 0;
        virtual void         download()                 = 0;
        
        virtual MasterList*  referenceMasterList()      = 0;
        virtual DisplayList* addReference()             = 0;
        virtual void         deleteReference()          = 0;
};


class DisplayListImpl: public DisplayList {
    private:
        char*               name;        
        SongList*           songList;
        int                 treeId;
        int                 parentId;
        int                 refCount;
                
    protected:
    	MasterList*         masterList; // Playlist swaps this around
    
        virtual MasterList*     referenceMasterList();
        virtual void            downloadFunction() throw(ConnectionException);
        virtual void            sortFunction();
        virtual unsigned        filterFunction(const char*);
        virtual const SongList* getSongList() const;

    public:
        DisplayListImpl(int);
        DisplayListImpl(const char*, DisplayList&);
        virtual ~DisplayListImpl();
        
        virtual const char*  getName()    const;
        virtual int          getSize()    const;
        virtual int          getTreeId()  const;
        virtual HWND         getHwnd()    const;
        virtual const Song*  getSong(int) const;
        virtual int          isAborted()  const;
        virtual void         clear();
        virtual void         display();
        virtual void         sort();
        virtual void         filter();
        virtual void         setHwnd(HWND hwnd);

        virtual void         play()       const;
        virtual void         enqueue()    const;
        virtual void         save()       const;
        virtual void         drop(POINT)  const;
        virtual void         abort()      const;
        virtual void         download() throw(ConnectionException);
        
        virtual DisplayList* addReference();
        virtual void         deleteReference();
};

#endif /*DISPLAYLIST_H*/
