#ifndef DISPLAYLIST_H
#define DISPLAYLIST_H

#include "HTTPGet.h"
#include "Song.h"
#include "SongList.h"


class MasterList {
    private:
        char*        name;
        SongList*    masterList;
        HWND         hwnd;
        int          refCount;
                
        virtual ~MasterList();

    protected:
        void         downloadFunction() throw(ConnectionException);

    public:
        MasterList(const char*);
        MasterList*  addReference();
        void         deleteReference();
        
        void         clear();
        void         download() throw(ConnectionException);
        void         setHwnd(HWND);
        
        int          getSize()        const { return masterList->getSize();         }
        Song*        getSong(int ndx) const { return masterList->getSong(ndx);      }
};


class DisplayList {
    public:
        virtual ~DisplayList() {}
        
        virtual const char*  getName()            const = 0;
        virtual int          getSize()            const = 0;
        virtual const Song*  getSong(int ndx)     const = 0;
        virtual void         clear()                    = 0;
        virtual void         search()                   = 0;
        virtual void         sort()                     = 0;
        virtual void         filter()                   = 0;
        virtual int          getTreeId()          const = 0;
        virtual void         setHwnd(HWND hwnd)         = 0;
        virtual void         play()               const = 0;
        virtual void         enqueue()            const = 0;
        virtual void         drop(POINT)          const = 0;
        virtual void         abort()              const = 0;
        virtual void         download()                 = 0;
        virtual MasterList*  getMasterList()            = 0;
};


class DisplayListImpl: public DisplayList {
    private:
        char*               name;
        MasterList*         masterList;
        SongList*           displayList;
        HWND                hwnd;
        int                 treeId;
                
    protected:
        virtual MasterList* getMasterList();
        virtual void        downloadFunction() throw(ConnectionException);
        virtual void        sortFunction();
        virtual unsigned    filterFunction(const char*);

    public:
        DisplayListImpl(int);
        DisplayListImpl(const char*, DisplayList&);
        virtual ~DisplayListImpl();
        
        virtual const char*  getName()    const;
        virtual int          getSize()    const;
        virtual const Song*  getSong(int) const;        
        virtual void         clear();
        virtual void         search();
        virtual void         sort();
        virtual void         filter();                        
        virtual void         download() throw(ConnectionException);
        virtual void         abort()      const;
        virtual void         play()       const;
        virtual void         enqueue()    const;
        virtual void         drop(POINT)  const;        
        virtual int          getTreeId()  const;
        virtual void         setHwnd(HWND hwnd);
};

#endif /*DISPLAYLIST_H*/
