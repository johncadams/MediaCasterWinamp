using namespace std;
#include <string>

#include <windows.h>
#include <shlobj.h>

#include "gen_ml/listview.h"
#include "gen_ml/childwnd.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "SongList.h"
#include "Configuration.h"
#include "defaultres.h"

#define TRACE_GLOBALS
#include "Trace.h"
#include "Messages.h"
#include "date.h"


#define SEARCH_DURATION      64
#define DOWNLOAD_DURATION   150
#define UPDATE_DURATION     150


static CasterLibrary*     library;
static int                hasLoaded     = 0;
static int                connFailed    = 0;
static int                extraFeatures = 0;
static int                adminFeatures = 0;
static int                downloading   = 0;
static int                treeId;
static HMENU              rootMenus;
static HMENU              childMenus;
static HMENU              listMenus;
static int                skinlistViewId;
static unexpected_handler unexpectedHandler;

// Globals
Configuration             configuration;
HTTPSession*              httpSession;
W_ListView                listView;

    
// These have to agree with Apache::MP3::Resample!
static char* bitrates[][2]=
{   {"24kbps", "stream=1;bitrate=24%20kbps"},
    {"40kbps", "stream=1;bitrate=40%20kbps"},
    {"56kbps", "stream=1;bitrate=56%20kbps"},
    {"64kbps", "stream=1;bitrate=64%20kbps"},
    {"96kbps", "stream=1;bitrate=96%20kbps"},
    {"Full",   "stream=1"},
};

static int bitratesSize = sizeof(bitrates)/sizeof(char*)/2;

#define LEFT	0x1000
#define TOP		0x0100
#define RIGHT	0x0010
#define BOTTOM	0x0001

static ChildWndResizeItem main_resize_rlist[] =
{ { MAIN_SEARCH_FIELD,     RIGHT},
  { MAIN_CLEAR_BTN,        LEFT  | RIGHT},
  { MAIN_LIST,             RIGHT | BOTTOM},
  { MAIN_PLAY_BTN,         TOP   | BOTTOM},
  { MAIN_ENQUEUE_BTN,      TOP   | BOTTOM},  
  { MAIN_REFRESH_BTN,      TOP   | BOTTOM},
  { MAIN_STATUS_TEXT,      TOP   | RIGHT | BOTTOM},
  { MAIN_SAVE_BTN,         LEFT  | TOP   | RIGHT | BOTTOM},
  { MAIN_CONFIG_BTN,       LEFT  | TOP   | RIGHT | BOTTOM},
};

static ChildWndResizeItem config_resize_rlist[] =
{ { CONFIG_HOST_FIELD,     RIGHT},
  { CONFIG_PORT_FIELD,     LEFT  | RIGHT},
  { CONFIG_USERNAME_FIELD, RIGHT | BOTTOM},
  { CONFIG_PASSWORD_FIELD, TOP   | BOTTOM},
};


static int  (*wad_getColor)              (int idx);
static int  (*wad_handleDialogMsgs)      (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam); 
static void (*wad_DrawChildWindowBorders)(HWND hwndDlg, int *tab, int tabsize);
static void (*cr_init)                   (HWND hwndDlg, ChildWndResizeItem *list, int num);
static void (*cr_resize)                 (HWND hwndDlg, ChildWndResizeItem *list, int num);



static int CenteredBox(HWND hwnd, const char* message, const char* title, int button) {
    TRACE("CenteredBox");
    unsigned len = 0;
    string   tmp = string(PLUGIN_NAME) +" - " +title;
    char*    dup = strdup(message);
    char*    tok = dup;
    char*    line;
    string   msg;
    
    while( (line=strtok(tok, "\n")) ) {
        len = len>strlen(line)?len:strlen(line);
        tok = NULL;
    }
    delete dup;
    
    tok = dup = strdup(message);    
    while( (line=strtok(tok, "\n")) ) {
        int l = (int)((len-strlen(line))*.75);
        string pad;
        for (int i=0; i<l; i++) pad += " ";
        msg += pad+line+"\n";
        tok = NULL;
    }
    delete dup;
    
    return MessageBox(hwnd, msg.c_str(), tmp.c_str(), button);
}


static void MsgBox(HWND hwnd, const char* message, const char* title) {
    TRACE("MsgBox");
    string tmp = string(PLUGIN_NAME) +" - " +title;
    MessageBox(hwnd, message, tmp.c_str(), MB_OK);
}


static int YesNoBox(HWND hwnd, const char* message, const char* title) {
    TRACE("YesNoBox");
    return CenteredBox(hwnd, message, title, MB_YESNO)==IDYES;
}


static int downloadUpgradeIfAvailableDialog(HWND hwnd) {
    TRACE("downloadUpgradeIfAvailableDialog");
    if (library->isUpgradeAvailable()) {
        if (YesNoBox(hwnd, UPGRADE_AVAIL_MSGBOX, "Upgrade")) {            
            library->downloadUpgrade();
            return true;
        }
    }
    return false;
}


void aboutBox(HWND hwnd) {
    TRACE("aboutBox");
    char features[1024];
    strcpy(features, COPYRIGHT_MSGBOX);
    strcat(features, "\n \n");
    strcat(features, "\nBuild: ");   strcat(features, getDateStr(configuration.getBuildDate()));
    strcat(features, "\nLogging: "); strcat(features, configuration.isLogging() ?"enabled":"disabled");
    strcat(features, "\nThreads: "); strcat(features, configuration.isThreaded()?"enabled":"disabled");
    strcat(features, "\nExtras: ");  strcat(features, extraFeatures?"enabled":"disabled");
    strcat(features, "\nAdmin: ");   strcat(features, adminFeatures?"enabled":"disabled");

    CenteredBox(hwnd, features, "About", MB_OK);
}


void fileIoProblemBox(HWND hwnd, const char* file, const char* reason) {
	TRACE("fileIoProblemBox");
	char msg[1024];
	sprintf(msg, ERROR_OPENING_FILE, file, reason);
    ::MsgBox(hwnd, msg, "Warning");
}


void connectionProblemBox(HWND hwnd, const char* reason) {
    TRACE("connectionProblemBox");
    
    char msg[1024];
    sprintf(msg, CONN_PROBLEM_MSGBOX, reason);
    
    ::setStatusMessage(hwnd, CONN_PROBLEM_STATUS);
    ::MsgBox(hwnd, msg, "Error");
    ::configDialog(hwnd);
}


void newFeatureBox(HWND hwnd, const char* reason, int& warned) {
    TRACE("newFeatureBox");
    if (!warned && !downloadUpgradeIfAvailableDialog(hwnd)) {
        string msg = string(reason) +"\n \n" CONTACT_US;
                 
        warned = 1;
        ::MsgBox(hwnd, msg.c_str(), "Error");
    }
}


void setStatusMessage(HWND hwnd, const char* status) {
//  TRACE("setStatusMessage");
    SetDlgItemText(hwnd, MAIN_STATUS_TEXT, status);
}


void showAbortButton(HWND hwnd, int show) {
    TRACE("showAbortButton");
    downloading = show;
    SetWindowText(GetDlgItem(hwnd,MAIN_REFRESH_BTN), show?"Abort":"Refresh");
}


void grayRefreshButton(HWND hwnd, int gray) {
    TRACE("grayRefreshButton");    
    EnableWindow(GetDlgItem(hwnd,MAIN_REFRESH_BTN),!gray);
}


void showExtraFeatures(HWND hwnd, int show) {
    TRACE("showExtraFeatures");
    
    extraFeatures = show;
    ShowWindow(GetDlgItem(hwnd, MAIN_SAVE_BTN),show);
    
	if (show) {
		DestroyMenu(listMenus);
		listMenus = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(MAIN_LIST_MENU));
	} else {
		RemoveMenu(listMenus, MAIN_LIST_SAVE_ITEM, MF_BYCOMMAND);
		showAdminFeatures(hwnd, false); // No extra features... no admin features
	}
}


void showAdminFeatures(HWND hwnd, int show) {
    TRACE("showAdminFeatures");
}


void grayDownloadButton(HWND hwnd, int gray) {
    // TRACE("grayDownloadButton");    
    EnableWindow(GetDlgItem(hwnd, MAIN_SAVE_BTN),!gray);
}


void getSearchString(HWND hwnd, char* str, unsigned strlen) {
    TRACE("getSearchString");
    GetDlgItemText(hwnd, MAIN_SEARCH_FIELD, str, strlen);
    str[strlen-1] = '\0';
}


void setConnectionFailed() {
    TRACE("setConnectionFailed");
    connFailed = 1;
}


void setConnectionSuccess() {
    TRACE("setConnectionSuccess");
    connFailed = 0;
}


int isConnectionFailed() {
    TRACE("isConnectionFailed");
    return connFailed;
}


void exceptionHandler() {
	TRACE("---Uncaught Exception Handler---");
	connectionProblemBox(plugin.hwndLibraryParent, "Unexpected exception caught");
	unexpectedHandler();
}


static int init() {
    TRACE("init");
    unexpectedHandler = set_unexpected(exceptionHandler);
    
    configuration.init(plugin);  
    if (tracePrinter.init(configuration.getLogfilePath(), configuration.isLogging())!=0) {
    	fileIoProblemBox(plugin.hwndLibraryParent, configuration.getLogfilePath(), strerror(errno));
    }
    
    // add our root item to the tree
    mlAddTreeItemStruct mla = {0,(char*)PLUGIN_NAME,1,};
    SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&mla,ML_IPC_ADDTREEITEM);
    treeId = mla.this_id;
    
    httpSession = new HTTPSession(configuration.getCacheDir(), configuration.getUser(), configuration.getPassword());
    library     = new CasterLibrary(treeId);
 
    // add our pop-up menus
    rootMenus  = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(TREE_ROOT_MENU));
    childMenus = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(TREE_CHILD_MENU));
    listMenus  = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(MAIN_LIST_MENU));
        
    // see if we have a pending error from a previous installer run
    if (configuration.getMessage()[0]) {
        
        char msg[4096];
        sprintf(msg, INSTALLER_INSTRUCTS,
                     configuration.getMessage(), 
                     configuration.getURL( configuration.getInstallerPath() ).c_str());

        MessageBox(plugin.hwndLibraryParent, msg, "Error", MB_OK);
        configuration.resetMessage();
        configuration.setAutoUpdate(0);
    }
    
    return 0;
}


static void quit() {
    TRACE("quit");
    delete library;    
    delete httpSession;
    set_unexpected(unexpectedHandler);
}


static VOID CALLBACK searchTimerCallback(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
	TRACE("searchTimerCallback");
    char filter[256];
    KillTimer(hwnd,id);
    GetDlgItemText(hwnd, MAIN_SEARCH_FIELD, filter, sizeof(filter));
    filter[255]=0;
    configuration.setFilter(filter);
    library->display();
}


static VOID CALLBACK downloadTimerCallback(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
	TRACE("downloadTimerCallback");
	KillTimer(hwnd,id);
    library->download();
}


static int CALLBACK folderSelectionDialogCallback(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData) {
	TRACE("folderSelectionDialogCallback");
	// If the BFFM_INITIALIZED message is received
	// set the path to the start path.
	switch (uMsg) {
		case BFFM_INITIALIZED: {
			if (!lpData) {
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
			}
		}
	}
	return 0;
}


char* folderSelectionDialog(HWND hwnd, const char* message, const char* currDir) {
	TRACE("folderSelectionDialog");
	BROWSEINFO   bi = { 0 };
	LPITEMIDLIST pidl;
	TCHAR        szDisplay[MAX_PATH];
	BOOL         retval;
	char         rtnPath[MAX_PATH];

	bi.hwndOwner      = hwnd;
	bi.pszDisplayName = szDisplay;
	bi.lpszTitle      = TEXT(message);
	bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn           = folderSelectionDialogCallback;
	bi.lParam         = (LPARAM) currDir;

	pidl = SHBrowseForFolder(&bi);

	if (NULL != pidl) {
		retval = SHGetPathFromIDList(pidl, rtnPath);
	} else {
		retval = FALSE;
	}

	if (!retval) {
		return NULL;
	}
	
	return strdup(rtnPath);
}


static BOOL CALLBACK advancedDialogCallback(HWND advDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TRACE("advancedDialogCallback");
	switch (uMsg) {
        case WM_INITDIALOG:
        	SetDlgItemText(advDlg, ADVANCED_SAVEDIR_FIELD, configuration.getSaveDir());
        	break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            	case ADVANCED_BROWSE_BTN:
            		char* dir;
            		dir = folderSelectionDialog(advDlg,MP3_DIRECTORY_CHOOSER,configuration.getSaveDir());
            		if (dir) SetDlgItemText(advDlg, ADVANCED_SAVEDIR_FIELD, dir);
            		break;
            		
            	case IDOK: {
                    TRACE("advancedDialogCallback::IDOK");
                    char tmp[1024];
                    GetDlgItemText(advDlg,ADVANCED_SAVEDIR_FIELD,tmp,sizeof(tmp));
                    configuration.setSaveDir(tmp);
                    EndDialog(advDlg,LOWORD(wParam) == IDOK);
                }
	        	case IDCANCEL:
		            EndDialog(advDlg,LOWORD(wParam) == IDOK);
            }
	}
	return 0;
}	

	
void advancedDialog(HWND hwnd) {
    TRACE("advancedDialog");
    DialogBox(plugin.hDllInstance,MAKEINTRESOURCE(ADVANCED_DIALOG),hwnd,advancedDialogCallback);
}


static BOOL CALLBACK configDialogCallback(HWND configDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // TRACE("configDialogCallback");
    switch (uMsg) {
        case WM_INITDIALOG: {
        	// Just to speed things up when we have connection problems
        	int         upgradeAvailable = false;
        	const char* upgradeMsg       = CONN_PROBLEM_STATUS2;
        	
        	SetTextColor(GetWindowDC(configDlg),wad_getColor?wad_getColor(WADLG_ITEMFG):RGB(0xff,0xff,0xff));
            SetBkColor  (GetWindowDC(configDlg),RGB(0x00,0xff,0x00));
        	if (cr_init) cr_init(configDlg,config_resize_rlist,sizeof(config_resize_rlist)/sizeof(config_resize_rlist[0]));
        	
        	if (!::isConnectionFailed()) {
        		upgradeAvailable = library->isUpgradeAvailable();
        		upgradeMsg       = library->getUpgradeAvailableStatus();
        	}
        	
            SetDlgItemText(configDlg, CONFIG_HOST_FIELD,     configuration.getHost());
            SetDlgItemText(configDlg, CONFIG_PORT_FIELD,     configuration.getPort());
            SetDlgItemText(configDlg, CONFIG_USERNAME_FIELD, configuration.getUser());
            SetDlgItemText(configDlg, CONFIG_PASSWORD_FIELD, configuration.getPassword());
            CheckDlgButton(configDlg, CONFIG_UPGRADE_CHECK,  configuration.isAutoUpdate()?BST_CHECKED:BST_UNCHECKED);
            
            int j = 0;
            for (int i=0; i<bitratesSize; i++) {
                SendDlgItemMessage(configDlg, CONFIG_BITRATE_SELECT, CB_INSERTSTRING, i, (LPARAM)bitrates[i][0]);
                if (strcmp(bitrates[i][1], configuration.getBitrate())==0) j = i;
            }
            SendDlgItemMessage(configDlg, CONFIG_BITRATE_SELECT, CB_SETCURSEL,        j,  0);
            SendDlgItemMessage(configDlg, CONFIG_PASSWORD_FIELD, EM_SETPASSWORDCHAR, '*', 0);
            
            SetDlgItemText(configDlg, CONFIG_UPGRADE_TEXT, upgradeMsg);
            EnableWindow(GetDlgItem(configDlg,CONFIG_UPGRADE_BTN), upgradeAvailable);            
            ShowWindow(GetDlgItem(configDlg,CONFIG_ADV_BTN), extraFeatures);
	      	            
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TRACE("configDialogCallback::IDOK");
                    // Update the globals
                    char tmp[1024];
                    int  changed = 0;
                    
                    GetDlgItemText(configDlg,CONFIG_HOST_FIELD,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getHost())!=0);
                     configuration.setHost(tmp);
                     
                    GetDlgItemText(configDlg,CONFIG_PORT_FIELD,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getPort())!=0);
                     configuration.setPort(tmp);
                     
                    GetDlgItemText(configDlg,CONFIG_USERNAME_FIELD,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getUser())!=0);
                     configuration.setUsername(tmp);
                     httpSession->setUsername(tmp);
                     
                    GetDlgItemText(configDlg,CONFIG_PASSWORD_FIELD,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getPassword())!=0);
                     configuration.setPassword(tmp);
                     httpSession->setPassword(tmp);
                     
                    GetDlgItemText(configDlg,CONFIG_BITRATE_SELECT,tmp,sizeof(tmp));
                     for (int i=0; i<bitratesSize; i++) {
                        if (strcmp(bitrates[i][0], tmp)==0) {
                            configuration.setBitrate(bitrates[i][1]);
                            break;
                        }
                    }   
                
                    configuration.setAutoUpdate( IsDlgButtonChecked(configDlg,CONFIG_UPGRADE_CHECK) );
                 
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    if (changed || isConnectionFailed()) {
                    	library->download();
                    }
                    break;                            
                }
                case CONFIG_REFRESH_BTN: {
                    TRACE("configDialogCallback::DLG_REFRESH");
                    library->clearCache();
					library->download();					
                    break;                            
                }    
                case CONFIG_EMPTY_BTN: {
                    TRACE("configDialogCallback::DLG_EMPTY");
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    library->clear();
                    break;  
                }
                case CONFIG_UPGRADE_BTN: {
                    TRACE("configDialogCallback::DLG_UPGRADE");                    
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    library->downloadUpgrade();
                    break;                               
                }
                case CONFIG_ADV_BTN: {
                	TRACE("configDialogCallback::DLG_UPGRADE");
					EndDialog(configDlg,LOWORD(wParam) == IDOK);
					advancedDialog(configDlg);
                	break;
                }
                case IDCANCEL:
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    break;                               
            }
        	break;
        
        case WM_SIZE:
        	if (wParam != SIZE_MINIMIZED) {
            	if (cr_resize) cr_resize(configDlg,config_resize_rlist,sizeof(config_resize_rlist)/sizeof(config_resize_rlist[0]));
        	}
        	break;
      
    	case WM_PAINT: {
            int tab[] = { CONFIG_HOST_FIELD    |DCW_SUNKENBORDER, 
            		      CONFIG_PORT_FIELD    |DCW_SUNKENBORDER,
            		      CONFIG_USERNAME_FIELD|DCW_SUNKENBORDER,
            		      CONFIG_PASSWORD_FIELD|DCW_SUNKENBORDER };
            if (wad_DrawChildWindowBorders) wad_DrawChildWindowBorders(configDlg,tab,4);
          	break;
        }      
    }
    return 0;
}


void configDialog(HWND hwnd) {
    TRACE("configDialog");
    DialogBox(plugin.hDllInstance,MAKEINTRESOURCE(CONFIG_DIALOG),hwnd,configDialogCallback);
}


static int onListItemClick(HWND hwndParent) {
    TRACE("onListItemClick");
    
    POINT p;
    GetCursorPos(&p);
    int r=TrackPopupMenu(GetSubMenu(listMenus,0),TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_NONOTIFY,p.x,p.y,0,hwndParent,NULL);

    switch (r) {
        case MAIN_LIST_PLAY_ITEM:
            library->play();
            break;
    
        case MAIN_LIST_ENQUEUE_ITEM:
            library->enqueue();
            break;
            
        case MAIN_LIST_SAVE_ITEM:
            library->save();
            break;
    }            
    return 1;
}


static BOOL CALLBACK mainPageCallback(HWND mainDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // TRACE("mainPageCallback");
    
    if (wad_handleDialogMsgs) {
        BOOL a=wad_handleDialogMsgs(mainDlg,uMsg,wParam,lParam); if (a) return a;
    }
    
    switch (uMsg) {
        case WM_DISPLAYCHANGE:
            ListView_SetTextColor  (listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMFG):RGB(0xff,0xff,0xff));
            ListView_SetBkColor    (listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMBG):RGB(0x00,0x00,0x00));
            ListView_SetTextBkColor(listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMBG):RGB(0x00,0x00,0x00));
            listView.refreshFont();
            return 0;
    
        case WM_INITDIALOG: {
            TRACE("mainPageCallback/WM_INITDIALOG");
            library->setTreeId(lParam, mainDlg);
        
            *(void**)&wad_getColor              =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,1, ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&wad_handleDialogMsgs      =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,2, ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&wad_DrawChildWindowBorders=(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,3, ML_IPC_SKIN_WADLG_GETFUNC);              
            *(void**)&cr_init                   =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,32,ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&cr_resize                 =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,33,ML_IPC_SKIN_WADLG_GETFUNC);
      
	      	showExtraFeatures(mainDlg, extraFeatures);  // turning off also	turns off admin features
	      	grayDownloadButton(mainDlg, true);  // This isn't working	      	

            if (cr_init) cr_init(mainDlg,main_resize_rlist,sizeof(main_resize_rlist)/sizeof(main_resize_rlist[0]));

            listView.setLibraryParentWnd(plugin.hwndLibraryParent);
            listView.setwnd(GetDlgItem(mainDlg,MAIN_LIST));
            listView.AddCol("Artist", 140);
            listView.AddCol("Title",  140);
            listView.AddCol("Album",  140);
            listView.AddCol("Length",  55);
            listView.AddCol("Track #", 55);
            listView.AddCol("Genre",   75);
            listView.AddCol("Year",    55);
            listView.AddCol("Comment",140);
            ListView_SetTextColor  (listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMFG):RGB(0xff,0xff,0xff));
            ListView_SetBkColor    (listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMBG):RGB(0x00,0x00,0x00));
            ListView_SetTextBkColor(listView.getwnd(),wad_getColor?wad_getColor(WADLG_ITEMBG):RGB(0x00,0x00,0x00));
            
            skinlistViewId = SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(int)listView.getwnd(),ML_IPC_SKIN_LISTVIEW);

            SetDlgItemText(mainDlg,MAIN_SEARCH_FIELD,configuration.getFilter());
            if (!hasLoaded || isConnectionFailed()) {
                // Defer the loading until we have completly initialized the screen
                hasLoaded = 1;
                SetTimer(0,0,DOWNLOAD_DURATION,downloadTimerCallback);
            }

            return 0;
        }
        case WM_NOTIFY: {            
            LPNMHDR l=(LPNMHDR)lParam;
            if (l->idFrom==MAIN_LIST) {

            	int count = ListView_GetSelectedCount(listView.getwnd());
            	grayDownloadButton(mainDlg,count==0);

            	if (l->code == (UINT)NM_RCLICK) {
            		TRACE("mainPageCallback/NM_RLCLK");
            		onListItemClick(mainDlg);
            		
            	} else if (l->code == (UINT)NM_DBLCLK) {
                    TRACE("mainPageCallback/NM_DBLCLK");
                    if (!!(GetAsyncKeyState(VK_SHIFT)&0x8000)) library->enqueue();
                    else                                       library->play();
            
                } else if (l->code == LVN_BEGINDRAG) {
                    SetCapture(mainDlg);
              
                } else if (l->code == LVN_ODFINDITEM) { // yay we find an item (for kb shortcuts)
                    TRACE("mainPageCallback/LVN_ODFINDITEM");
                    NMLVFINDITEM *t = (NMLVFINDITEM *)lParam;
                    int i=t->iStart;
                    if (i >= library->getSize()) i=0;
            
                    int cnt=library->getSize()-i;
                    if (t->lvfi.flags & LVFI_WRAP) cnt+=i;
    
                    while (cnt-->0) {
                        const Song* thissong = library->getSong(i);
                        char   tmp[256];
                        string name;
    
                        switch (configuration.getSortColumn()) {
                            case COL_ARTIST:   name=thissong->artist;  break;
                            case COL_TITLE:    name=thissong->title;   break;
                            case COL_ALBUM:    name=thissong->album;   break;
                            case COL_GENRE:    name=thissong->genre;   break;                    
                            case COL_FILENAME: name=thissong->file;    break;
                            case COL_COMMENT:  name=thissong->comment; break;
                            case COL_LENGTH:   wsprintf(tmp,"%d:%02d",thissong->songlen/60,(thissong->songlen)%60); name=tmp;                          break;
                            case COL_TRACK:    if (thissong->track > 0    && thissong->track < 1000) { wsprintf(tmp,"%d",thissong->track); name=tmp; } break;
                            case COL_YEAR:     if (thissong->year  < 5000 && thissong->year  > 0)    { wsprintf(tmp,"%d",thissong->year ); name=tmp; } break;
                        }
    
                        if (t->lvfi.flags & (4|LVFI_PARTIAL)) {
                            if (!strnicmp(name.c_str(),t->lvfi.psz,strlen(t->lvfi.psz))) {
                                SetWindowLong(mainDlg,DWL_MSGRESULT,i);
                                return 1;
                            }
                      
                        } else if (t->lvfi.flags & LVFI_STRING) {
                            if (!stricmp(name.c_str(),t->lvfi.psz)) {
                                SetWindowLong(mainDlg,DWL_MSGRESULT,i);
                                return 1;
                            }
                      
                        } else {
                            SetWindowLong(mainDlg,DWL_MSGRESULT,-1);
                            return 1;
                        }
                
                        if (++i == library->getSize()) i=0;
                    }
                    SetWindowLong(mainDlg,DWL_MSGRESULT,-1);
                    return 1;
                        
                } else if (l->code == LVN_GETDISPINFO) {                             
                    NMLVDISPINFO *lpdi = (NMLVDISPINFO*) lParam;
                    int item=lpdi->item.iItem;
    
                    if (item < 0 || item >= library->getSize()) return 0;
                    const Song* thissong = library->getSong(item);
        
                    if (lpdi->item.mask & (LVIF_TEXT|0)) {
                        if (lpdi->item.mask & LVIF_TEXT) {
                            char   tmpbuf[256];
                            string name;
                        
                            switch (lpdi->item.iSubItem) {
                                case COL_ARTIST:   name=thissong->artist;  break;
                                case COL_TITLE:    name=thissong->title;   break;
                                case COL_ALBUM:    name=thissong->album;   break;
                                case COL_GENRE:    name=thissong->genre;   break;
                                case COL_FILENAME: name=thissong->file;    break;
                                case COL_COMMENT:  name=thissong->comment; break;
                                case COL_LENGTH:   wsprintf(tmpbuf,"%d:%02d",thissong->songlen/60,(thissong->songlen)%60); name=tmpbuf;                   break;
                                case COL_TRACK:    if (thissong->track>0 && thissong->track<1000) { wsprintf(tmpbuf,"%d",thissong->track); name=tmpbuf; } break;
                                case COL_YEAR:     if (thissong->year>0  && thissong->year<5000)  { wsprintf(tmpbuf,"%d",thissong->year);  name=tmpbuf; } break;
                            }              
                            lstrcpyn(lpdi->item.pszText,name.c_str(),lpdi->item.cchTextMax);
                        }
                    }
                    return 0;
              
                } else if (l->code == LVN_COLUMNCLICK) {
                    TRACE("mainPageCallback/LVN_COLUMNCLICK");
                    NMLISTVIEW *p=(NMLISTVIEW*)lParam;
                    if (p->iSubItem == configuration.getSortColumn()) {
                        configuration.reverseDirection();
                    } else {
                        configuration.setSortColumn(p->iSubItem);
                    }
                    library->sort();
                }      
            }
        } 
        break;
    
    	case WM_COMMAND:
	    	{ TRACE("mainPageCallback/WM_COMMAND"); }
        	switch(LOWORD(wParam)) {
            	case MAIN_CLEAR_BTN: {
                	TRACE("mainPageCallback/DLG_CLEAR");
	                SetDlgItemText(mainDlg,MAIN_SEARCH_FIELD,"");
	                break;
	            }
	            case MAIN_PLAY_BTN: {
	                TRACE("mainPageCallback/DLG_PLAY");
	                library->play();
	                break;
	            }
	            case MAIN_SAVE_BTN: {
	                TRACE("mainPageCallback/DLG_SAVE");
	                library->save();
	                // This resets the status message in 30secs
	                SetTimer(0,0, 30*1000, searchTimerCallback);
	                break;
	            }
	            case MAIN_ENQUEUE_BTN: {
	                TRACE("mainPageCallback/DLG_ENQUEUE");
	                library->enqueue();
	                break;
	            }
	            case MAIN_REFRESH_BTN: {
	                TRACE("mainPageCallback/DLG_REFRESH2");
	                if (downloading) {
	                	library->abort();
	                } else {
	                	library->clearCache();
						library->download();
	                }
	                break;
	            }
	            case MAIN_CONFIG_BTN: {
	                TRACE("mainPageCallback/DLG_CONFIG");
	                configDialog(mainDlg);
	                break;
	            }
	            case MAIN_SEARCH_FIELD:                
	                if (HIWORD(wParam) == EN_CHANGE) {
	                    TRACE("mainPageCallback/DLG_SEARCH");
	                    SetTimer(0,0,SEARCH_DURATION,searchTimerCallback);
	                }
	                break;
	        }
			break;
      
	    case WM_SIZE:
	        if (wParam != SIZE_MINIMIZED) {
	            if (cr_resize) cr_resize(mainDlg,main_resize_rlist,sizeof(main_resize_rlist)/sizeof(main_resize_rlist[0]));
	        }
	        break;
	      
	    case WM_PAINT: {
	            int tab[] = { MAIN_SEARCH_FIELD|DCW_SUNKENBORDER, MAIN_LIST|DCW_SUNKENBORDER};
	            if (wad_DrawChildWindowBorders) wad_DrawChildWindowBorders(mainDlg,tab,2);
	        }
	        return 0;
	      
	    case WM_DESTROY:
	        SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,skinlistViewId,ML_IPC_UNSKIN_LISTVIEW);
	        return 0;

	    case WM_LBUTTONUP:
	        if (GetCapture() == mainDlg) {
	            TRACE("mainPageCallback/WM_LBUTTONUP");
	            ReleaseCapture();
	
	            POINT p;
	            p.x=GET_X_LPARAM(lParam);
	            p.y=GET_Y_LPARAM(lParam);
	            ClientToScreen(mainDlg,&p);
	    
	            HWND h=WindowFromPoint(p);
	            if (h != mainDlg && !IsChild(mainDlg,h)) {
	                library->drop(p);
	            }           
	        }
	        break;
      
	    case WM_MOUSEMOVE:
	        if (GetCapture()==mainDlg) {
	            POINT p;
	            p.x=GET_X_LPARAM(lParam);
	            p.y=GET_Y_LPARAM(lParam);
	            ClientToScreen(mainDlg,&p);
	            mlDropItemStruct m={ML_TYPE_ITEMRECORDLIST,NULL,0};
	            m.p=p;
	            HWND h=WindowFromPoint(p);
	            if (!h || h == mainDlg || IsChild(mainDlg,h)) {
	                SetCursor(LoadCursor(NULL,IDC_NO));
	            } else {       
	                SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&m,ML_IPC_HANDLEDRAG);
	            }
	        }
	        break;    
	}  
    return 0;
}


static BOOL CALLBACK authDialogCallback(HWND authDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            TRACE("authDialogCallback/WM_INITDIALOG");
            SetDlgItemText(authDlg, AUTH_USERNAME_FIELD,configuration.getUser());
            SetDlgItemText(authDlg, AUTH_PASSWORD_FIELD,configuration.getPassword()); 
            SendDlgItemMessage(authDlg, AUTH_PASSWORD_FIELD, EM_SETPASSWORDCHAR, '*', 0);       
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TRACE("authDialogCallback/IDOK");
                    char tmp[64];
                    GetDlgItemText(authDlg,AUTH_USERNAME_FIELD,tmp,sizeof(tmp));
                     configuration.setUsername(tmp);
                     httpSession->setUsername(tmp);
                    GetDlgItemText(authDlg,AUTH_PASSWORD_FIELD,tmp,sizeof(tmp));
                     configuration.setPassword(tmp);
                     httpSession->setPassword(tmp);      
                    library->download();
                    EndDialog(authDlg,LOWORD(wParam) == IDOK);                  
                    break; 
                }    
                case IDCANCEL: {
                    TRACE("authDialogCallback/IDCANCEL");
                    EndDialog(authDlg,LOWORD(wParam) == IDOK);
                    break;         
                }                    
            }
    }
    return 0;
}
 
 
void authDialog(HWND hwnd) {
    ::setStatusMessage(hwnd, "Authorization required");
    DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(AUTH_DIALOG), hwnd, authDialogCallback);
}
  
  
static int onTreeItemClick(int selTreeId, int action, HWND hwndParent) {
    if (action == ML_ACTION_RCLICK) {
        TRACE("onTreeItemClick/ML_ACTION_RCLICK");
        POINT p;
        int   r;
        HMENU menu;

        if (treeId == selTreeId) menu = GetSubMenu(rootMenus, 0);
        else                     menu = GetSubMenu(childMenus,0);
        
       	GetCursorPos(&p);
        r = TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_NONOTIFY,p.x,p.y,0,hwndParent,NULL);
        
        switch (r) {
            case TREE_ABOUT_ITEM:
                aboutBox(hwndParent);
                break;
        
            case TREE_CONFIG_ITEM:
                configDialog(hwndParent);
                break;
                
            case TREE_PLAY_ITEM: {
            	TRACE("onTreeItemClick/TREE_PLAY_ITEM");
            	SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)selTreeId,ML_IPC_SETCURTREEITEM);
            	library->play();
            	break;
            }            	
            case TREE_ENQUEUE_ITEM: {
            	TRACE("onTreeItemClick/TREE_ENQUEUE_ITEM");
            	SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)selTreeId,ML_IPC_SETCURTREEITEM);
				library->enqueue();
            	break;
            }            
        }            
    }
    return 1;
}


static int pluginMessageProc(int message_type, int param1, int param2, int param3) {
    // check for any global messages here
    if (message_type == ML_MSG_CONFIG) {
        TRACE("pluginMessageProc/ML_MSG_CONFIG");
        configDialog(plugin.hwndLibraryParent);
        return true;
    
    } else if (message_type >= ML_MSG_TREE_BEGIN && message_type <= ML_MSG_TREE_END) {
        TRACE("pluginMessageProc/ML_MSG_TREE");
        // This checks to see if the select tree item is one of ours
        if (library->checkId(param1)) {        
            switch (message_type) {
                case ML_MSG_TREE_ONCREATEVIEW: {
                    TRACE("pluginMessageProc/ML_MSG_TREE_ONCREATEVIEW");
                    return (int)CreateDialogParam(plugin.hDllInstance,MAKEINTRESOURCE(MAIN_DIALOG),(HWND)param2,mainPageCallback,param1);
                }
                case ML_MSG_TREE_ONCLICK:
                    return onTreeItemClick(param1,param2,(HWND)param3);
            }
        }
  } else if (message_type == ML_MSG_ONSENDTOBUILD) {
        if (param1 == ML_TYPE_ITEMRECORDLIST) {
            TRACE("pluginMessageProc/ML_TYPE_ITEMRECORDLIST");  
            mlAddToSendToStruct s;
            s.context=param2;
            s.desc=PLUGIN_NAME;
            s.user32=(int)pluginMessageProc;
            SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&s,ML_IPC_ADDTOSENDTO);
        }    
  }

  return false;
}



static CasterLibrary* getLibrary() {
	return library;
}	

MediaCasterMediaLibraryPlugin::MediaCasterMediaLibraryPlugin() {
	MediaCasterMediaLibraryPlugin::version     = MLHDR_VER;
	MediaCasterMediaLibraryPlugin::description = PLUGIN_FULLNAME;
	MediaCasterMediaLibraryPlugin::init        = ::init;
	MediaCasterMediaLibraryPlugin::quit        = ::quit;
	MediaCasterMediaLibraryPlugin::MessageProc = ::pluginMessageProc;
	MediaCasterMediaLibraryPlugin::getLibrary  = ::getLibrary;
}

MediaCasterMediaLibraryPlugin plugin;

winampMediaLibraryPlugin* winampGetMediaLibraryPlugin() {
    	return (winampMediaLibraryPlugin*)&plugin;
}
