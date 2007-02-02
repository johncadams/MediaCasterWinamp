
#include <malloc.h>
#include "Song.h"

#undef DO_TRACING
#include "Trace.h"


Song::Song(const char* artist, const char* title,   const char* album, const char* genre, 
           const char* file,   const char* songlen, const char* track, const char* year,
           const char* comment) {
    TRACE("Song::Song");
    Song::artist   = artist ?artist :"";
    Song::title    = title  ?title  :"";
    Song::album    = album  ?album  :"";
    Song::genre    = genre  ?genre  :"";
    Song::file     = file   ?file   :"";
    Song::comment  = comment?comment:"";
    Song::songlen  = atoi(songlen?songlen:"");
    Song::track    = atoi(track  ?track  :"");
    Song::year     = atoi(year   ?year   :"");
    Song::refCount = 1;
}


Song::~Song() {
    TRACE("Song::~Song");   
}


Song* Song::addReference() {
    TRACE("Song::addReference");
    this->refCount++;
    return this;
}


void Song::deleteReference() {
    TRACE("Song::deleteReference");
    refCount--;
    LOGGER("refCount",refCount);
    if (refCount==0) delete this;    
}
 
        
string Song::toUrl(string host, string port, string query) const {
    TRACE("Song::toUrl");
    string url = string("http://") ; 
    if (host.length()  >0)   url += host;
    if (port.length()  >0)   url += ":" +port;
    if (file.length()  >0)   url += file;
    if (query.length() >0)   url += "?" +query;
    
    return url;                            
}


void Song::toListItem(string host, string port, string bitrate, itemRecord& item) const {
    TRACE("Song::toListItem");
    string url = toUrl(host, port, bitrate);
    
    memset(&item,0,sizeof(itemRecord));
    item.album    = strdup(album  .c_str());
    item.artist   = strdup(artist .c_str());
    item.title    = strdup(title  .c_str());
    item.genre    = strdup(genre  .c_str());
    item.filename = strdup(url    .c_str());
    item.comment  = strdup(comment.c_str());
    item.track    =        track;
    item.year     =        year;
    item.length   =        songlen;
}
