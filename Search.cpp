#include <string.h>
#include "Trace.h"
#include "Search.h"


// columns names used for searching (same as winamp)
#define COL_ARTIST_NAME		"artist"
#define COL_TITLE_NAME      "title"
#define COL_ALBUM_NAME      "album"
#define COL_LENGTH_NAME     "length"
#define COL_TRACK_NAME      "trackno"
#define COL_GENRE_NAME      "genre"
#define COL_YEAR_NAME       "year"
#define COL_COMMENT_NAME    "comment"
#define COL_FILENAME_NAME   "filename"

#define HAS_OP_NAME			"has"
#define NOTHAS_OP_NAME		"nothas"
#define EQ_OP_NAME1			"="
#define EQ_OP_NAME2			"=="
#define NOTEQ_OP_NAME		"!="
#define GT_OP_NAME			">"
#define LT_OP_NAME			"<"
#define GE_OP_NAME			">="
#define LE_OP_NAME			"<="
#define BEGINS_OP_NAME		"begins"
#define ENDS_OP_NAME		"ends"
#define EMPTY_OP_NAME		"isempty"
#define NOTEMPTY_OP_NAME	"isnotempty"

#define HAS_OP				1
#define NOTHAS_OP			2
#define EQ_OP				3
#define NOTEQ_OP			4
#define GT_OP				5
#define LT_OP				6
#define GE_OP				7
#define LE_OP				8
#define BEGINS_OP			9
#define ENDS_OP				10
#define EMPTY_OP			11
#define NOTEMPTY_OP			12

#define OR_EXPR1			"or"
#define OR_EXPR2			"|"
#define OR_EXPR3			"||"
#define AND_EXPR1			"and"
#define AND_EXPR2			"&"
#define AND_EXPR3			"&&"
#define NOT_EXPR1			"not"
#define NOT_EXPR2			"!"


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


static int simpleSearch(const char* expr, const Song* song) {
//	TRACE("simpleSearch");  This gets called to many times
	char  year [32] = "";
    char  len  [32] = "";
    char  track[32] = "";
    
    sprintf(year, "%d",song->year);
    sprintf(len,  "%d",song->songlen);
    sprintf(track,"%d",song->track);
    
    for (const char* p=expr; *p; p+=strlen(p)+1) {
		if (
	    	!::in_string(song->title  .c_str(),p) &&
			!::in_string(song->artist .c_str(),p) &&                 
			!::in_string(song->album  .c_str(),p) &&                     
	        !::in_string(song->genre  .c_str(),p) &&
	        !::in_string(song->comment.c_str(),p) &&
	        !::in_string(year,                 p) &&
	        !::in_string(track,                p) &&
	        !::in_string(len,                  p)) {
	        	
	        return false;
		}
    }
    return true;
}


static int advancedSearch(const char* expr, const Song* song) {
//	TRACE("advancedSearch");  This gets called to many times
	char  year [32] = "";
    char  len  [32] = "";
    char  track[32] = "";
        
	int fnd  = 1;
	int andd = 1;
    int knot = 0;
    int test = 0;
    
    sprintf(year, "%d",song->year);
    sprintf(len,  "%d",song->songlen);
    sprintf(track,"%d",song->track);
    
    if (song->year   ==0) year [0] = '\0';
    if (song->songlen==0) len  [0] = '\0';
    if (song->track  ==0) track[0] = '\0';
        	
    for (const char* p=expr; *p; p+=strlen(p)+1) {
    	
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
		
		const char* field = p;	p+=strlen(p)+1;
		const char* opStr = p;
		const char* value;
	
		if      (strcmp(COL_TITLE_NAME,   field)==0) value = song->title  .c_str();
		else if (strcmp(COL_ARTIST_NAME,  field)==0) value = song->artist .c_str();
		else if (strcmp(COL_ALBUM_NAME,   field)==0) value = song->album  .c_str();
		else if (strcmp(COL_GENRE_NAME,   field)==0) value = song->genre  .c_str();
		else if (strcmp(COL_COMMENT_NAME, field)==0) value = song->comment.c_str();
		else if (strcmp(COL_YEAR_NAME,    field)==0) value = year;
		else if (strcmp(COL_TRACK_NAME,   field)==0) value = track;
		else if (strcmp(COL_LENGTH_NAME,  field)==0) value = len;
		else                                         return 0; // Unknown field
	
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
		else                                         return 0; // Unknown operator
		       
	
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
    }
    return fnd;
}


unsigned doSearch(MasterList* masterList, SongList* songList, const char* filter) {
	unsigned length = 0;
	char expr[300];

	::parseQuickSearch(expr, filter);
	songList->clear();
	
	int   adv = expr[0]=='?';
	char* buf = expr;        
    if (adv) {
    	if (expr[1] == 0) buf+=strlen(buf)+1; // Advance p over the '?' marker
    	else              buf++;
    }
	
	for (int i=0; i<masterList->getSize(); i++) {        
        Song* song = masterList->getSong(i);
        int   fnd  = 0;
        
        if (adv) fnd = advancedSearch(buf, song);
        else     fnd = simpleSearch  (buf, song);
        
        if (fnd) {
        	length += song->songlen;
	        songList->addSong( song->addReference() );
        }
	}
	
	return length;
}
