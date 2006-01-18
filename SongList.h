#ifndef SONGLIST_H
#define SONGLIST_H

#include "gen_ml/itemlist.h"
#include "Song.h"


class SongList {
    private:
        C_ItemList* songList;
        
    protected:
        static const int PLAY    = 0;
        static const int ENQUEUE = !PLAY;
        
        void         playOrEnqueue(int) const;

    public:
        SongList();
        virtual ~SongList();
        
        int          getSize()            const { return songList->GetSize();           }
        Song*        getSong(int ndx)     const;
        Song*        getSong(const char*) const;
        void         addSong(Song* song);
        void         purge();
        void         sort();
        
        void         play()               const { playOrEnqueue(PLAY);                  }
        void         enqueue()            const { playOrEnqueue(ENQUEUE);               }
        int          toItemRecordList(itemRecordList&, int) const;
};

#endif /*SONGLIST_H*/
