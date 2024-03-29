#include <string>
#include <windows.h>
#include "gen_ml/ml.h"

using namespace std;


#ifndef SONG_H
#define SONG_H


class Song {
    private:
        int refCount;
        
    protected:
        virtual ~Song();
        
    public:
        string artist;
        string title;
        string album;
        string genre;
        string file;
        string comment;
        int    songlen;
        int    track;
        int    year;        
    
        Song(const char*, const char*, const char*, const char*, const char*, const char*,
             const char*, const char*, const char*);                     
        
        Song*  addReference();
        void   deleteReference();
        
        string toUrl     (string, string, string) const;
        void   toListItem(string, string, string, itemRecord&) const;
};

#endif
