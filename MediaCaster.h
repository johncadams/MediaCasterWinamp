#ifndef MEDIACASTER_H_
#define MEDIACASTER_H_

#include <windows.h>
#include "gen_ml/ml.h"
#include "gen_ml/listview.h"
#include "Configuration.h"
#include "CasterLibrary.h"

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






class MediaCasterMediaLibraryPlugin: public winampMediaLibraryPlugin {
	public:
		MediaCasterMediaLibraryPlugin();
		CasterLibrary* (*getLibrary)(void);
};


extern "C" {
    __declspec( dllexport ) winampMediaLibraryPlugin* winampGetMediaLibraryPlugin();
};

extern W_ListView                    listView;
extern Configuration                 configuration;
extern HTTPSession*                  httpSession;
extern MediaCasterMediaLibraryPlugin plugin;


extern void  authDialog              (HWND);
extern void  configDialog            (HWND);
extern void  aboutBox                (HWND);
extern void  fileIoProblemBox        (HWND, const char*, const char*);
extern void  connectionProblemBox    (HWND, const char*);
extern void  newFeatureBox           (HWND, const char*, int&);
extern char* folderSelectionDialog   (HWND, const char*);
extern void  setStatusMessage        (HWND, const char*);
extern void  showAbortButton         (HWND, int);
extern void  grayRefreshButton       (HWND, int);
extern void  showDownloadButton      (HWND, int);
extern void  grayDownloadButton      (HWND, int);
extern void  getSearchString         (HWND, char*, unsigned);
extern void  setConnectionFailed     ();
extern void  setConnectionSuccess    ();
extern int   isConnectionFailed      ();

#endif /*MEDIACASTER_H_*/
