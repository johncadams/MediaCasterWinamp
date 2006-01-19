/*
** Copyright (C) 2003 Nullsoft, Inc.
**
** This software is provided 'as-is', without any express or implied warranty. In no event will
**  the authors be held liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial 
** applications, and to alter it and redistribute it freely, subject to the following 
** restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you 
**      wrote the original software.  If you use this software in a product, an acknowledgment
**      in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented
**      as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
** Copyright (C) 2005 Smada Nhoj Industries
**
*/

using namespace std;
#include <string>

#include <windows.h>

#include "gen_ml/listview.h"
#include "gen_ml/childwnd.h"
#include "gen_ml/itemlist.h"
#include "winamp/wa_dlg.h"

#include "MediaCaster.h"
#include "SongList.h"
#include "CasterLibrary.h"
#include "Configuration.h"
#include "defaultres.h"

#define TRACE_GLOBALS
#include "Trace.h"


// Timer IDs
#define SEARCH_TIMER        500
#define DOWNLOAD_TIMER      600
#define UPGRADE_TIMER       700

#define SEARCH_DURATION      64
#define DOWNLOAD_DURATION   150
#define UPDATE_DURATION     150


static CasterLibrary* library;
static int            hasLoaded  = 0;
static int            connFailed = 0;
static int            treeId;
static HMENU          rootMenus;
static int            skinlistViewId;

// Globals
Configuration         configuration;
W_ListView            listView;


    
// These have to agree with Apache::MP3::Resample!
static char* bitrates[][2]=
{   "24kbps", "stream=1;bitrate=24%20kbps",
    "40kbps", "stream=1;bitrate=40%20kbps",
    "56kbps", "stream=1;bitrate=56%20kbps",
    "64kbps", "stream=1;bitrate=64%20kbps",
    "96kbps", "stream=1;bitrate=96%20kbps",
    "Full",   "stream=1",
};

static int bitratesSize = sizeof(bitrates)/sizeof(char*)/2;

static ChildWndResizeItem resize_rlist[]=
{ { DLG_SEARCH,  0x0010},
  { DLG_CLEAR,   0x1010},
  { DLG_LIST,    0x0011},
  { DLG_PLAY,    0x0101},
  { DLG_ENQUEUE, 0x0101},
  { DLG_REFRESH2,0x0101},
  { DLG_ABORT,   0x0101},
  { DLG_STATUS,  0x0111},
  { DLG_CONFIG,  0x1111},
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
    
    while(line=strtok(tok, "\n")) {
        len = len>strlen(line)?len:strlen(line);
        tok = NULL;
    }
    delete dup;
    
    tok = dup = strdup(message);    
    while(line=strtok(tok, "\n")) {
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
    CenteredBox(hwnd, message, title, MB_OK);
}


static int YesNoBox(HWND hwnd, const char* message, const char* title) {
    TRACE("YesNoBox");
    return CenteredBox(hwnd, message, title, MB_YESNO)==IDYES;
}


static int downloadUpgradeIfAvailableDialog(HWND hwnd) {
    TRACE("downloadUpgradeIfAvailableDialog");
    if (library->isUpgradeAvailable()) {
        string mess = string("A newer version of ") +PLUGIN_NAME +" is available";
        mess += "\n \nWould you like to upgrade?\n";
        if (YesNoBox(hwnd, mess.c_str(), "Upgrade")) {
            library->downloadUpgrade();
            return true;
        }
    }
    return false;
}


void aboutBox(HWND hwnd) {
    TRACE("aboutBox");
    MsgBox(hwnd, PLUGIN_FULLNAME"\n \n"COPYRIGHT, "About");
}


void connectionProblemBox(HWND hwnd, const char* reason) {
    TRACE("connectionProblemBox");
    string msg = "There was a problem connecting to the server";
    msg += string("\n \nCause: ") +reason;
    
    ::setStatusMessage(hwnd, "Connection FAILURE");
    ::MsgBox(hwnd, msg.c_str(), "Connection Error");
    ::configDialog(hwnd);
}


void newFeatureBox(HWND hwnd, const char* reason, int& warned) {
    TRACE("newFeatureBox");
    if (!warned && !downloadUpgradeIfAvailableDialog(hwnd)) {
        string msg = string(reason) +
                     "\n \n"                                    \
                     "Contact DigitalStreams,\n"                \
                     "a wholly owned subsidary of Smada Nhoj Industries.";
                 
        warned = 1;
        ::MsgBox(hwnd, msg.c_str(), "Feature Not Available Error");
    }
}


void setStatusMessage(HWND hwnd, const char* status) {
//  TRACE("setStatusMessage");
    SetDlgItemText(hwnd, DLG_STATUS, status);
}


void showAbortButton(HWND hwnd, int show) {
    TRACE("showAbortButton");    
    ShowWindow(GetDlgItem(hwnd, DLG_REFRESH2),!show);
    ShowWindow(GetDlgItem(hwnd, DLG_ABORT),    show);    
}


void grayRefreshButton(HWND hwnd, int gray) {
    TRACE("grayRefreshButton");    
    EnableWindow(GetDlgItem(hwnd, DLG_REFRESH2),!gray);
}


void getSearchString(HWND hwnd, char* str, unsigned strlen) {
    TRACE("getSearchString");
    GetDlgItemText(hwnd, DLG_SEARCH, str, strlen);
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


int init() {
    TRACE("init");
    configuration.load(plugin);  
    
    // add our root item to the tree
    mlAddTreeItemStruct mla = {0,(char*)PLUGIN_NAME,1,};
    SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,(WPARAM)&mla,ML_IPC_ADDTREEITEM);
    treeId = mla.this_id;
    
    library = new CasterLibrary(treeId);
    
    // add our pop-up menus
    rootMenus = LoadMenu(plugin.hDllInstance,MAKEINTRESOURCE(IDR_POPUPMENU));
    
    // see if we have a pending error from a previous installer run
    if (configuration.getMessage()[0]) {
        string url = configuration.getURL( configuration.getInstallerPath() );
        string msg = string("The auto-installer failed, you will need to run the installer manully:\n")+
                     "   Cause: "+configuration.getMessage() +"\n\n"+
                     "Workaround:\n"+
                     "   1) Terminate Winamp\n"+
                     "   2) Download the installer ("+url+")\n"+
                     "   3) Execute the installer\n"+
                     "   4) Re-enable Auto-Update (Config dialog)";
               
        MessageBox(plugin.hwndLibraryParent, msg.c_str(), "Error", MB_OK);
        configuration.resetMessage();
        configuration.setAutoUpdate(0);
    }
    
    return 0;
}


void quit() {
    TRACE("quit");
    delete library;
}


static BOOL CALLBACK configDialogCallback(HWND configDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // TRACE("configDialogCallback");
    switch (uMsg) {
        case WM_INITDIALOG: {
            SetDlgItemText(configDlg, DLG_HOST,       configuration.getHost());
            SetDlgItemText(configDlg, DLG_PORT,       configuration.getPort());
            SetDlgItemText(configDlg, DLG_USERNAME,   configuration.getUser());
            SetDlgItemText(configDlg, DLG_PASSWORD,   configuration.getPassword());
            SetDlgItemText(configDlg, DLG_CONNECTED,  "");
            CheckDlgButton(configDlg, DLG_AUTOUPGRADE,configuration.isAutoUpdate()?BST_CHECKED:BST_UNCHECKED);
            
            int j = 0;
            for (int i=0; i<bitratesSize; i++) {
                SendDlgItemMessage(configDlg, DLG_BITRATE, CB_INSERTSTRING, i, (LPARAM)bitrates[i][0]);
                if (strcmp(bitrates[i][1], configuration.getBitrate())==0) j = i;
            }
            SendDlgItemMessage(configDlg, DLG_BITRATE,  CB_SETCURSEL,        j,  0);
            SendDlgItemMessage(configDlg, DLG_PASSWORD, EM_SETPASSWORDCHAR, '*', 0);
            
            HWND button = GetDlgItem(configDlg, DLG_UPGRADE);
            if (library->isUpgradeAvailable()) {
                SetDlgItemText(configDlg, DLG_UPDTEXT, "An upgrade is available");
                EnableWindow(button, 1);
            } else {
                SetDlgItemText(configDlg, DLG_UPDTEXT, "Your installation is current");
                EnableWindow(button, 0);
            }
            
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TRACE("configDialogCallback::IDOK");
                    // Update the globals
                    char tmp[1024];
                    int  changed = 0;
                    
                    GetDlgItemText(configDlg,DLG_HOST,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getHost())!=0);
                     configuration.setHost(tmp);
                     
                    GetDlgItemText(configDlg,DLG_PORT,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getPort())!=0);
                     configuration.setPort(tmp);
                     
                    GetDlgItemText(configDlg,DLG_USERNAME,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getUser())!=0);
                     configuration.setUser(tmp);
                     
                    GetDlgItemText(configDlg,DLG_PASSWORD,tmp,sizeof(tmp));
                     changed |= (strcmp(tmp,configuration.getPassword())!=0);
                     configuration.setPassword(tmp);
    
                    GetDlgItemText(configDlg,DLG_BITRATE,tmp,sizeof(tmp));
                     for (int i=0; i<bitratesSize; i++) {
                        if (strcmp(bitrates[i][0], tmp)==0) {
                            configuration.setBitrate(bitrates[i][1]);
                            break;
                        }
                    }   
                
                    configuration.setAutoUpdate( IsDlgButtonChecked(configDlg,DLG_AUTOUPGRADE) );
                 
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    if (changed || isConnectionFailed()) {
                        library->download();
                    }
                    break;                            
                }
                case DLG_REFRESH: {
                    TRACE("configDialogCallback::DLG_REFRESH");
                    library->download();
                    break;                            
                }    
                case DLG_EMPTY: {
                    TRACE("configDialogCallback::DLG_EMPTY");
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    library->clear();
                    break;  
                }
                case DLG_UPGRADE: {
                    TRACE("configDialogCallback::DLG_UPGRADE");                    
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    library->downloadUpgrade();
                    break;                               
                }
                case IDCANCEL:
                    EndDialog(configDlg,LOWORD(wParam) == IDOK);
                    break;                               
            }
        return 0;
    }
    return 0;
}


void configDialog(HWND hwnd) {
    TRACE("configDialog");
    DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(IDD_CONFIGDIALOG), hwnd, configDialogCallback);
}


static BOOL CALLBACK mainPageCallback(HWND mainDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//  TRACE("mainPageCallback");
    
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
        
            *(void**)&wad_getColor              =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,1,ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&wad_handleDialogMsgs      =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,2,ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&wad_DrawChildWindowBorders=(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,3,ML_IPC_SKIN_WADLG_GETFUNC);              
            *(void**)&cr_init                   =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,32,ML_IPC_SKIN_WADLG_GETFUNC);
            *(void**)&cr_resize                 =(void*)SendMessage(plugin.hwndLibraryParent,WM_ML_IPC,33,ML_IPC_SKIN_WADLG_GETFUNC);
      
            if (cr_init) cr_init(mainDlg,resize_rlist,sizeof(resize_rlist)/sizeof(resize_rlist[0]));

            listView.setLibraryParentWnd(plugin.hwndLibraryParent);
            listView.setwnd(GetDlgItem(mainDlg,DLG_LIST));
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

            SetDlgItemText(mainDlg,DLG_SEARCH,configuration.getFilter());
            if (!hasLoaded || isConnectionFailed()) {
                // Defer the loading until we have completly initialized the screen
                hasLoaded = 1;
                SetTimer(mainDlg,DOWNLOAD_TIMER,DOWNLOAD_DURATION,NULL);
            }

            return 0;
        }
        case WM_NOTIFY: {            
            LPNMHDR l=(LPNMHDR)lParam;
            if (l->idFrom==DLG_LIST) {
                if (l->code == (UINT)NM_DBLCLK) {
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
        switch(LOWORD(wParam)) {
            case DLG_CLEAR: {
                TRACE("mainPageCallback/DLG_CLEAR");
                SetDlgItemText(mainDlg,DLG_SEARCH,"");
                break;
            }
            case DLG_PLAY: {
                TRACE("mainPageCallback/DLG_PLAY");
                library->play();
                break;
            }
            case DLG_ENQUEUE: {
                TRACE("mainPageCallback/DLG_ENQUEUE");
                library->enqueue();
                break;
            }
            case DLG_REFRESH2: {
                TRACE("mainPageCallback/DLG_REFRESH2");
                library->download();
                break;
            }
            case DLG_ABORT: {
                TRACE("mainPageCallback/DLG_ABORT");
                library->abort();                
                break;
            }
            case DLG_CONFIG: {
                TRACE("mainPageCallback/DLG_CONFIG");
                configDialog(mainDlg);
                break;
            }
            case DLG_SEARCH:                
                if (HIWORD(wParam) == EN_CHANGE) {
                    TRACE("mainPageCallback/DLG_SEARCH");
                    KillTimer(mainDlg,SEARCH_TIMER);
                    SetTimer (mainDlg,SEARCH_TIMER,SEARCH_DURATION,NULL);
                }
                break;
        }
        break;
      
    case WM_TIMER:
        if (wParam == SEARCH_TIMER) {
            TRACE("mainPageCallback/SEARCH_TIMER");
            KillTimer(mainDlg,SEARCH_TIMER);
            char filter[256];
            GetDlgItemText(mainDlg,DLG_SEARCH,filter,sizeof(filter));
            filter[255]=0;
            configuration.setFilter(filter);
            library->display();
            
        } else if (wParam == DOWNLOAD_TIMER) {
            TRACE("mainPageCallback/DOWNLOAD_TIMER");
            KillTimer(mainDlg,DOWNLOAD_TIMER);
            library->download();

        } else if (wParam == UPGRADE_TIMER) {
            TRACE("mainPageCallback/UPGRADE_TIMER");
            KillTimer(mainDlg,UPGRADE_TIMER);
            library->downloadUpgrade();
        }
        break;
        
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            if (cr_resize) cr_resize(mainDlg,resize_rlist,sizeof(resize_rlist)/sizeof(resize_rlist[0]));
        }
        break;
      
    case WM_PAINT: {
            int tab[] = { DLG_SEARCH|DCW_SUNKENBORDER, DLG_LIST|DCW_SUNKENBORDER};
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
            SetDlgItemText(authDlg, DLG_USERNAME,configuration.getUser());
            SetDlgItemText(authDlg, DLG_PASSWORD,configuration.getPassword()); 
            SendDlgItemMessage(authDlg, DLG_PASSWORD, EM_SETPASSWORDCHAR, '*', 0);       
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TRACE("authDialogCallback/IDOK");
                    char tmp[64];
                    GetDlgItemText(authDlg,DLG_USERNAME,tmp,sizeof(tmp));
                     configuration.setUser(tmp);                    
                    GetDlgItemText(authDlg,DLG_PASSWORD,tmp,sizeof(tmp));
                     configuration.setPassword(tmp);      
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
    DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(IDD_AUTHDIALOG), hwnd, authDialogCallback);
}
  
  
static int onTreeItemClick(int param, int action, HWND hwndParent) {
    if (action == ML_ACTION_RCLICK) {
        TRACE("onTreeItemClick/ML_ACTION_RCLICK");
        POINT p;
        GetCursorPos(&p);
        int r=TrackPopupMenu(GetSubMenu(rootMenus,0),TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_NONOTIFY,p.x,p.y,0,hwndParent,NULL);
    
        switch (r) {
            case ID_ABOUT:
                aboutBox(hwndParent);
                break;
        
            case ID_CONFIG:
                configDialog(hwndParent);
                break;
        }            
    }
    return 1;
}


int pluginMessageProc(int message_type, int param1, int param2, int param3) {
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
                    return (int)CreateDialogParam(plugin.hDllInstance,MAKEINTRESOURCE(ID_VIEW),(HWND)param2,mainPageCallback,param1);
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


winampMediaLibraryPlugin plugin = {
	MLHDR_VER,
	PLUGIN_FULLNAME,
	init,
	quit,
    pluginMessageProc,
};


extern "C" {
    __declspec( dllexport ) winampMediaLibraryPlugin* winampGetMediaLibraryPlugin() {
        return &plugin;
    }    
};
