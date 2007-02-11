#include <stdio.h>

#include <windows.h>
#include "gen_ml/listview.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "Messages.h"
#include "SongList.h"
#include "Trace.h"
#include "date.h"


static int sortFunction(const void* elem1, const void* elem2) {
//  TRACE("sortFunction");  This gets called to many times
    Song* a = (Song*)*(void**)elem1;
    Song* b = (Song*)*(void**)elem2;

    int use_by  = configuration.getSortColumn();
    int use_dir = configuration.getSortDirection();

    #define RETIFNZ(v) if ((v)<0) return use_dir?1:-1; if ((v)>0) return use_dir?-1:1;

    // this might be too slow, but it'd be nice
    for (int x = 0; x < 4; x ++) {
        if (use_by == COL_YEAR) { // year -> artist -> album -> track
            int v1 = a->year;
            int v2 = b->year;
            if (v1<0)v1=0;
            if (v2<0)v2=0;
            RETIFNZ(v1-v2)
            use_by = COL_ARTIST;     
      
        } else if (use_by == COL_TITLE) { // title -> artist -> album -> track
            int v = stricmp(a->title.c_str(),b->title.c_str());
            RETIFNZ(v)
            use_by = COL_ARTIST;
      
        } else if (use_by == COL_ARTIST) { // artist -> album -> track -> title
            int v = stricmp(a->artist.c_str(),b->artist.c_str());
            RETIFNZ(v)
            use_by = COL_ALBUM;
      
        } else if (use_by == COL_ALBUM) { // album -> track -> title -> artist
            int v = stricmp(a->album.c_str(),b->album.c_str());
            RETIFNZ(v)
            use_dir = 0;
            use_by  = COL_TRACK;
      
        } else if (use_by == COL_GENRE) { // genre -> artist -> album -> track
            int v = stricmp(a->genre.c_str(),b->genre.c_str());
            RETIFNZ(v)
            use_by = COL_ARTIST;
            
        } else if (use_by == COL_COMMENT) { // genre -> artist -> album -> track
            int v = stricmp(a->comment.c_str(),b->comment.c_str());
            RETIFNZ(v)
            use_by = COL_ARTIST;
      
        } else if (use_by == COL_TRACK) { // track -> title -> artist -> album
            int v1 = a->track;
            int v2 = b->track;
            if (v1<0)v1=0;
            if (v2<0)v2=0;
            RETIFNZ(v1-v2)
            use_by = COL_TITLE;     
      
        } else if (use_by == COL_LENGTH) { // length -> artist -> album -> track
            int v1 = a->songlen;
            int v2 = b->songlen;
            if (v1<0)v1=0;
            if (v2<0)v2=0;
            RETIFNZ(v1-v2)
            use_by = COL_ARTIST;
      
        } else {
            break; // no sort order?
        }
    } 
    #undef RETIFNZ
    return 0;
}



SongList::SongList() {
    TRACE("SongList::SongList");
    SongList::songList = new C_ItemList();
}


SongList::~SongList() {
    TRACE("SongList::SongList");
    SongList::clear();
    delete songList;
}


Song* SongList::getSong(int ndx) const {
//  TRACE("SongList::getSong");
    return (Song*)songList->Get(ndx);
}


Song* SongList::getSong(const char* path) const {
//  TRACE("SongList::getSong");
    int i=songList->GetSize();
    while (i>0) {
        Song* song = (Song*)songList->Get(--i);
        if (strcmp(song->file.c_str(), path)==0) return song;
    }
    return NULL;
}

void SongList::addSong(Song* song) {
//  TRACE("SongList::addSong");
    songList->Add(song);
}


void SongList::clear() {
    TRACE("SongList::clear");
    if (songList) {
        int i=songList->GetSize();
        LOGGER("size",i);
        while (i>0) {
            Song* song = (Song*)songList->Get(--i);
            song->deleteReference();
            songList->Del(i);
        }
    }
}


void SongList::sort() {
    TRACE("SongList::sort");
    ::qsort(songList->GetAll(), songList->GetSize(), sizeof(void*) ,::sortFunction);  
}


void SongList::playOrEnqueue(int enqueue) const {   
    TRACE("SongList::playOrEnqueue");
    itemRecordList recList={0,};
    int playsong = SongList::toItemRecordList(recList, enqueue);
    if (recList.Size) {
        mlSendToWinampStruct s = {ML_TYPE_ITEMRECORDLIST,&recList,1};
        
        if (!enqueue) {
            SendMessage(plugin.hwndWinampParent, WM_WA_IPC, 0, IPC_DELETE);
        }

        SendMessage(plugin.hwndLibraryParent, WM_ML_IPC, (WPARAM)&s, ML_IPC_SENDTOWINAMP);

        if (!enqueue) {
            SendMessage(plugin.hwndWinampParent, WM_WA_IPC, playsong, IPC_SETPLAYLISTPOS);
            SendMessage(plugin.hwndWinampParent, WM_COMMAND,40045,0); // play button
        }
    }
    delete recList.Items; 
}

void SongList::save(HWND hwnd) const {
    TRACE("SongList::save"); 
    
    try {
	    int  i = 0;	    
		for (int x=0; x<songList->GetSize(); x++) {
			int numSel = ListView_GetSelectedCount(listView.getwnd());
	        if (listView.GetSelected(x)) {
	            const Song* song = (Song*)songList->Get(x);
	            string      url  = song->toUrl(configuration.getHost(), 
	                                           configuration.getPort(), 
	                                           "");
	                                           
				char*   filename = decodeURL( basename(url.c_str()) );
				string  path     = configuration.getSaveDir();
				
				if (path.length() == 0) {
					char* dir = folderSelectionDialog(hwnd, MP3_DIRECTORY_CHOOSER, NULL);
					if (!dir) {
						return;
					}
					configuration.setSaveDir(dir);
					path = configuration.getSaveDir();
					delete dir;
				}
				
				if (path[path.length()-1] != '\\') path.append("\\");
				path.append(filename);
	            HTTPGet httpGet(httpSession,url,path.c_str());
			    httpGet.addHeader("User-Agent: MediaCaster (Winamp)");
			    httpGet.addHeader("Accept:     text/*");
			    
			    if (i==0) setStatusMessage(hwnd, CONNECTING);
			    httpGet.connect();
			    
			    char bytes[1024];
			    int  cnt;
	    		int  total = 0;
	    		i++;
			    while( (cnt=httpGet.read(bytes, sizeof(bytes))) ) {
	        		total += cnt;	
	        		char status[256];        		
	        		sprintf(status, MP3_DOWNLOAD, i, numSel, int( float(total*100./httpGet.contentLen())) );
	        		setStatusMessage(hwnd, status);         
	    		}    		
	    		delete filename;
	        }
		}
		setStatusMessage(hwnd, MP3_DOWNLOAD_DONE); 
	        
    } catch (HTTPAuthenticationException& ex) {
        CATCH(ex);
        ::setConnectionFailed();        
        ::authDialog(hwnd);
    
	} catch (ConnectionException& ex) {
        CATCH(ex);
        ::setConnectionFailed();
        ::connectionProblemBox(hwnd, ex.getError());        
    }        
}

	
int SongList::toItemRecordList(itemRecordList& recList, int enqueue) const {
    TRACE("SongList::toItemRecordList");    
    recList.Alloc = recList.Size = 0;
    recList.Items = 0;
    
    int numSel   = 0;
    int firstSel = 0;
    for (int x=0; x<songList->GetSize(); x++) {
        if (listView.GetSelected(x)) {
            if (!numSel) firstSel = x;
            if (++numSel>1) break;
        }
    }
    
    // enqueue/0 selected - entire list
    //        /1 selected - selection
    //        /+ selected - selection
    // play   /0 selected - entire list (return 0)
    // play   /1 selected - entire list (return ndx)
    // play   /+ selected - selection   (return 0)
    for (int x=0; x<songList->GetSize(); x++) {
        if (numSel==0 || !enqueue&&numSel==1 || listView.GetSelected(x)) {
            allocRecordList(&recList,recList.Size+1,1024);
    
            const Song* song = (Song*)songList->Get(x);
            song->toListItem(configuration.getHost(), 
                             configuration.getPort(),
                             configuration.getBitrate(),
                             recList.Items[recList.Size]);
            recList.Size++;
        }
    }
    return numSel==1?firstSel:0;
}
