#include "gen_ml/listview.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "DisplayList.h"
#include "HTTPGet.h"
#include "Process.h"
#include "Trace.h"
#include "Messages.h"


static void parseQuickSearch(char* out, const char* in) {
    TRACE("parseQuickSearch");
    int inquotes = 0;
    int neednull = 0;

    while (*in) {
        char c = *in++;
        if (c != ' ' && c != '\t' && c != '\"') {
            neednull = 1;
            *out++   = c;
      
        } else if (c == '\"') {
            inquotes=!inquotes;
            if (!inquotes) {
                *out++   = 0;
                neednull = 0;
            }
      
        } else {
            if      (inquotes) *out++=c;
            else if (neednull) {
                *out++   = 0;
                neednull = 0;
            }
        }
    }
    *out++ = 0;
    *out++ = 0;
}


static int in_string(const char* string, const char* substring) {
//  TRACE("in_string");  This gets called to many times
    if (!string)     return 0;
    if (!*substring) return 1;
    int l=strlen(substring);
    while (string[0]) if (!strnicmp(string++,substring,l)) return 1; 
    return 0;
}



MasterList::MasterList(const char* name, SongList* songList) {
    TRACE("MasterList::MasterList");
    MasterList::name           = strdup(name);
    MasterList::songList       = songList;
    MasterList::refCount       = 1;
    JNL::open_socketlib();
}


MasterList::~MasterList() {
    TRACE("MasterList::~MasterList");
    delete songList;
    delete name;
    JNL::close_socketlib();
}


MasterList* MasterList::addReference() {
    TRACE("MasterList::addReference");
    this->refCount++;
    LOGGER("refCount",refCount);
    return this;
}


void MasterList::deleteReference() {
    TRACE("MasterList::deleteReference");
    refCount--;
    LOGGER("refCount",refCount);
    if (refCount==0) delete this;
}


void MasterList::setHwnd(HWND hwnd) {
    TRACE("MasterList::setHwnd");
    MasterList::hwnd = hwnd;
}


void MasterList::clear() {
    TRACE("MasterList::clear");
    songList->clear();
}


void MasterList::download() throw(ConnectionException) {
    TRACE("MasterList::download");
    downloadFunction();
}


void MasterList::downloadFunction() throw(ConnectionException)  {
    TRACE("MasterList::downloadFunction");
    if (configuration.getHost()[0]=='\0') return;
        
    stopLoading = 0;
    
    SongList* newSongs      = new SongList();
    string    masterListUrl = configuration.getURL( configuration.getLibraryPath() );

    setStatusMessage(hwnd, CONNECTING);
    HTTPGet httpGet(masterListUrl, configuration.getUser(), configuration.getPassword());
    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
    httpGet.addHeader("Accept:     text/*");
    
    try {           
        httpGet.connect();
        char* buf;
        int   total = 0;
        int   cnt;

        while ( !stopLoading && (cnt=httpGet.readLine(buf)) > 0) {
        	int   ndx = 0;
            char* fld[9];
            fld[0] = buf;
            for (char* p=buf; *p; p++) {
            	if (*p=='|') {
            		*p = 0;
            		fld[++ndx] = p+1;
            	}
            }
            
			char* title   = fld[0];
            char* artist  = fld[1];
            char* album   = fld[2];
            char* genre   = fld[3];
            char* year    = fld[4];
            char* track   = fld[5];
            char* len     = fld[6];
            char* file    = fld[7];
            char* comment = fld[8];
            newSongs->addSong( new Song(artist, title, album, genre, file, len, track, year, comment) );
            
            char status[256];
            total += cnt+1;
            sprintf(status, DISPLIST_DOWNLOAD, int( float(total*100./httpGet.contentLen())) );
            setStatusMessage(hwnd, status);
            delete buf;
        }

    } catch (HTTPAuthenticationException& ex) {
        // Have to do it this way or this exception isn't rethrown correctly
        CATCH(ex);
        delete newSongs;
        songList->clear();
        RETHROW(ex);

    } catch (ConnectionException& ex) {
    	CATCH(ex);
        delete newSongs;
        songList->clear();
        RETHROW(ex);
    }

    if (stopLoading) {
        delete newSongs;
    } else {
        delete songList;
        songList = newSongs;
    }
    
    LOGGER("size", songList->getSize());
}




DisplayListImpl::DisplayListImpl(int treeId) {
    TRACE("DisplayListImpl::DisplayListImpl");
    DisplayListImpl::name        = strdup("ROOT");
    DisplayListImpl::masterList  = new MasterList(name, new SongList());
    DisplayListImpl::songList    = new SongList();
    DisplayListImpl::treeId      = treeId;
    DisplayListImpl::parentId    = 0;
    DisplayListImpl::refCount    = 1;
}


DisplayListImpl::DisplayListImpl(const char* name, DisplayList& clone) {
    TRACE("DisplayListImpl::DisplayListImpl (clone)");
    DisplayListImpl::name        = strdup(name);
    DisplayListImpl::masterList  = clone.referenceMasterList();
    DisplayListImpl::songList    = new SongList();
    DisplayListImpl::refCount    = 1;
                
    // add the item to the tree
    mlAddTreeItemStruct mla = {clone.getTreeId(),(char*)name,1,};
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&mla, ML_IPC_ADDTREEITEM);
    treeId   = mla.this_id;
    parentId = clone.getTreeId();
}


DisplayListImpl::~DisplayListImpl() {
    TRACE("DisplayListImpl::~DisplayListImpl");
    
    SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, treeId, ML_IPC_DELTREEITEM);
    
    delete songList;
    masterList->deleteReference();
    delete name;
}


MasterList* DisplayListImpl::referenceMasterList() {
    TRACE("DisplayListImpl::referenceMasterList");
    return masterList->addReference();
}


DisplayList* DisplayListImpl::addReference() {
    TRACE("DisplayListImpl::addReference");
    this->refCount++;
	LOGGER("refCount",refCount);
    return this;
}


void DisplayListImpl::deleteReference() {
    TRACE("DisplayListImpl::deleteReference");
    refCount--;
    LOGGER("refCount",refCount);
    if (refCount==0) delete this;
}


const char* DisplayListImpl::getName() const {
    TRACE("DisplayListImpl::getName");
    return DisplayListImpl::name;
}


const SongList* DisplayListImpl::getSongList() const {
//  TRACE("DisplayListImpl::getSongList");
    return songList;
}


int DisplayListImpl::getSize() const {
//  TRACE("DisplayListImpl::getSize");
    return songList->getSize();
}


const Song* DisplayListImpl::getSong(int i) const {
//  TRACE("DisplayListImpl::getSong");
    return songList->getSong(i);
}


int DisplayListImpl::getTreeId() const {
    TRACE("DisplayListImpl::getTreeId");
    return DisplayListImpl::treeId;
}


void DisplayListImpl::download() throw(ConnectionException) {
    TRACE("DisplayListImpl::download");
    downloadFunction();
}


void DisplayListImpl::abort() const {
    TRACE("DisplayListImpl::abort");
    masterList->abort();
}


int DisplayListImpl::isAborted() const {
//  TRACE("DisplayListImpl::abort");
    return masterList->isAborted();
}


void DisplayListImpl::clear() {
    TRACE("DisplayListImpl::clear");
    songList->clear();
    masterList->clear();
}


HWND DisplayListImpl::getHwnd() const {
//  TRACE("DisplayListImpl::getHwnd");
    return masterList->getHwnd();
}


void DisplayListImpl::setHwnd(HWND hwnd) {
    TRACE("DisplayListImpl::setHwnd");
    masterList->setHwnd(hwnd);
}


void DisplayListImpl::sortFunction() {
    TRACE("DisplayListImpl::sortFunction");
    songList->sort();
}


void DisplayListImpl::sort() {
    TRACE("DisplayListImpl::sort");
    sortFunction();
    ListView_SetItemCount(listView.getwnd(),0);
    ListView_SetItemCount(listView.getwnd(),songList->getSize());
    ListView_RedrawItems (listView.getwnd(),0,songList->getSize()-1);
}


unsigned DisplayListImpl::filterFunction(const char* filter) {
    TRACE("DisplayListImpl::filterFunction");
    char filteritems[300];

	LOGGER("filter", filter);
	::parseQuickSearch(filteritems, filter);
	
    unsigned length = 0;
    songList->clear();
    for (int i=0; i<masterList->getSize(); i++) {        
        Song* song     = masterList->getSong(i);
        char  year[32] = "";
        char  len[32]  = "";
        char  track[32]= "";
        int   fnd      = 1;        
        int   adv      = filteritems[0]=='?';
        char* buf      = filteritems;

        if (adv) {
        	if (filteritems[1] == 0) buf+=strlen(buf)+1; // Advance p over the '?' marker
        	else                     buf++;
        }
        
        sprintf(year, "%d",song->year);
        sprintf(len,  "%d",song->songlen);
        sprintf(track,"%d",song->track);
        
        if (song->year   ==0) year [0] = '\0';
        if (song->songlen==0) len  [0] = '\0';
        if (song->track  ==0) track[0] = '\0';

        for (char* p=buf; *p; p+=strlen(p)+1) {
        	int andd = 1;
        	int knot = 0;
        	int test = 0;
        
			if (adv) {
				// Look for an and/or condition
				if (stricmp(p,OR_EXPR1) ==0 || stricmp(p,OR_EXPR2) ==0 || stricmp(p,OR_EXPR3) ==0 ||
				    stricmp(p,AND_EXPR1)==0 || stricmp(p,AND_EXPR2)==0 || stricmp(p,AND_EXPR3)==0 ) {
					if (stricmp(p,OR_EXPR1) ==0 || stricmp(p,OR_EXPR2) ==0 || stricmp(p,OR_EXPR3)==0) andd = 0;
					p+=strlen(p)+1;
				}
				
				if (stricmp(p,NOT_EXPR1)==0 || stricmp(p,NOT_EXPR2)==0) {
					if (stricmp(p,NOT_EXPR1)==0 || stricmp(p,NOT_EXPR2)==0) knot = 1;
					p+=strlen(p)+1;
				}
				
        		char*       field = p;	p+=strlen(p)+1;
        		char*       opStr = p;
        		const char* value;

        		if      (strcmp(COL_TITLE_NAME,   field)==0) value = song->title  .c_str();
        		else if (strcmp(COL_ARTIST_NAME,  field)==0) value = song->artist .c_str();
        		else if (strcmp(COL_ALBUM_NAME,   field)==0) value = song->album  .c_str();
        		else if (strcmp(COL_GENRE_NAME,   field)==0) value = song->genre  .c_str();
        		else if (strcmp(COL_COMMENT_NAME, field)==0) value = song->comment.c_str();
        		else if (strcmp(COL_YEAR_NAME,    field)==0) value = year;
        		else if (strcmp(COL_TRACK_NAME,   field)==0) value = track;
        		else if (strcmp(COL_LENGTH_NAME,  field)==0) value = len;
        		else                                         continue; // Unknown field

        		int op;
        		if      (stricmp(EMPTY_OP_NAME,   opStr)==0) op = EMPTY_OP;
        		else if (stricmp(NOTEMPTY_OP_NAME,opStr)==0) op = NOTEMPTY_OP;
        		else if (stricmp(HAS_OP_NAME,     opStr)==0) op = HAS_OP;
        		else if (stricmp(NOTHAS_OP_NAME,  opStr)==0) op = NOTHAS_OP;
        		else if (stricmp(EQ_OP_NAME1,     opStr)==0) op = EQ_OP;
        		else if (stricmp(EQ_OP_NAME2,     opStr)==0) op = EQ_OP;
        		else if (stricmp(GT_OP_NAME,      opStr)==0) op = GT_OP;
        		else if (stricmp(LT_OP_NAME,      opStr)==0) op = LT_OP;
        		else if (stricmp(GE_OP_NAME,      opStr)==0) op = GE_OP;
        		else if (stricmp(LE_OP_NAME,      opStr)==0) op = LE_OP;
        		else if (stricmp(NOTEQ_OP_NAME,   opStr)==0) op = NOTEQ_OP;
        		else if (stricmp(BEGINS_OP_NAME,  opStr)==0) op = BEGINS_OP;
        		else if (stricmp(ENDS_OP_NAME,    opStr)==0) op = ENDS_OP;
        		else                                         continue; // Unknown operator
           		       

				if (op!=EMPTY_OP && op!=NOTEMPTY_OP) {
					p+=strlen(p)+1;					
				}
	
				if (op==EMPTY_OP    && strlen(value)==0 				|| 
				    op==NOTEMPTY_OP && strlen(value)>0					||
                    op==HAS_OP      &&  ::in_string(value,p)			||
	    		    op==NOTHAS_OP   && !::in_string(value,p)	     	||
	    		    op==EQ_OP       && strcmp (value,p)==0				||
	    		    op==NOTEQ_OP    && strcmp (value,p)!=0				||
	    		    op==GT_OP       && strcmp (value,p)>0				||
	    		    op==LT_OP       && strcmp (value,p)<0				||
	    		    op==GE_OP       && strcmp (value,p)>=0				||
	    		    op==LE_OP       && strcmp (value,p)<=0				||
	    		    op==BEGINS_OP   && strncmp(value,p,strlen(p))==0	||
	    		    op==ENDS_OP     && strcmp (value+strlen(value)-strlen(p),p)==0) {
								
					test = 1;
	            			
				} else {
					test = 0;
			    }
			    
			    if (knot) test = !test;
			    fnd = andd ? fnd&&test : fnd||test;
		
	    	// search against everything
			} else {
				if (
			    	!::in_string(song->title  .c_str(),p) &&
					!::in_string(song->artist .c_str(),p) &&                 
					!::in_string(song->album  .c_str(),p) &&                     
			        !::in_string(song->genre  .c_str(),p) &&
			        !::in_string(song->comment.c_str(),p) &&
			        !::in_string(year,                 p) &&
			        !::in_string(track,                p) &&
			        !::in_string(len,                  p)) {
			        	
			        fnd = 0;
			        break;
			    }
			}
    	}
    	
	    if (fnd) {
	        length += song->songlen;
	        songList->addSong( song->addReference() );
        }
    }
    return length;
}


void DisplayListImpl::filter() {
    TRACE("DisplayListImpl::filter");
    char filter[256];
    unsigned filterlen = 0;  
    
    ::getSearchString(getHwnd(), filter, sizeof(filter));
    filterlen = filterFunction(filter);
    
    char status[256];
    int  days  = filterlen/86400;
    int  count = filterlen%86400;
    int  hours = count/3600;
    int  mins  =(count/60)%60;
    int  secs  = count%60;
    if (days==0) wsprintf(status, NUMITEMS_NODAYS, songList->getSize(),     hours,mins,secs);
    else         wsprintf(status, NUMITEMS_WDAYS,  songList->getSize(),days,hours,mins,secs);
    ::setStatusMessage(getHwnd(), status);
}


void DisplayListImpl::play() const {
    TRACE("DisplayListImpl::play");
    songList->play();
}


void DisplayListImpl::enqueue() const {
    TRACE("DisplayListImpl::enqueue");
    songList->enqueue();
}

        
void DisplayListImpl::drop(POINT p) const {
    TRACE("DisplayListImpl::drop");
    mlDropItemStruct m={ML_TYPE_ITEMRECORDLIST,NULL,0};
    m.p=p;
    m.flags=ML_HANDLEDRAG_FLAG_NOCURSOR;
    SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDRAG);

    if (m.result>0) {
        itemRecordList recList={0,}; 
        songList->toItemRecordList(recList, 1);
        if (recList.Size) {
            m.flags  = 0;
            m.result = 0;
            m.data   = &recList;                 
            SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDROP);
        }
        delete recList.Items;
    }
}


void DisplayListImpl::downloadFunction() throw(ConnectionException) {
    TRACE("DisplayListImpl::downloadFunction");
    masterList->download();
    DisplayListImpl::display();
    ListView_SetItemCount(listView.getwnd(), songList->getSize());    
}


void DisplayListImpl::display() {
    TRACE("DisplayListImpl::display");
    filter();
    sort();    
}
