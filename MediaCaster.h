#ifndef MEDIACASTER_H_
#define MEDIACASTER_H_

#include <windows.h>
#include "gen_ml/ml.h"
#include "gen_ml/listview.h"
#include "Configuration.h"

#define PLUGIN_NAME         "Media Caster"
#define PLUGIN_FULLNAME     PLUGIN_NAME" v"MC_VERSION


// columns in our big treeview
#define COL_ARTIST          0
#define COL_TITLE           1
#define COL_ALBUM           2
#define COL_LENGTH          3
#define COL_TRACK           4
#define COL_GENRE           5
#define COL_YEAR            6
#define COL_COMMENT         7
#define COL_FILENAME        8

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
#define EQ_OP_NAME			"="
#define NOTEQ_OP_NAME		"!="
#define BEGINS_OP_NAME		"begins"
#define ENDS_OP_NAME		"ends"
#define EMPTY_OP_NAME		"isempty"
#define NOTEMPTY_OP_NAME	"isnotempty"

#define HAS_OP				1
#define NOTHAS_OP			2
#define EQ_OP				3
#define NOTEQ_OP			4
#define BEGINS_OP			5
#define ENDS_OP				6
#define EMPTY_OP			7
#define NOTEMPTY_OP			6

#define OP_EXPR				"or"
#define AND_EXPR			"and"
#define NOT_EXPR			"not"


extern W_ListView               listView;
extern Configuration            configuration;
extern winampMediaLibraryPlugin plugin;


extern void authDialog              (HWND);
extern void configDialog            (HWND);
extern void aboutBox                (HWND);
extern void connectionProblemBox    (HWND, const char*);
extern void newFeatureBox           (HWND, const char*, int&);
extern void setStatusMessage        (HWND, const char*);
extern void showAbortButton         (HWND, int);
extern void grayRefreshButton       (HWND, int);
extern void getSearchString         (HWND, char*, unsigned);
extern void setConnectionFailed     ();
extern void setConnectionSuccess    ();
extern int  isConnectionFailed      ();

#endif /*MEDIACASTER_H_*/
