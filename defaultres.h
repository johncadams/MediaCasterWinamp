/* Weditres generated include file. Do NOT edit */
#include <windows.h>

#define	TREE_ROOT_MENU	1000
#define	TREE_ABOUT_ITEM	1001
#define	TREE_CONFIG_ITEM 1002

#define	TREE_CHILD_MENU	2000
#define	TREE_PLAY_ITEM	2001
#define	TREE_ENQUEUE_ITEM	2002

#define	MAIN_LIST_MENU	3000
#define	MAIN_LIST_PLAY_ITEM	3001
#define	MAIN_LIST_ENQUEUE_ITEM	3002

#define	ABOUT_DIALOG	4000
#define ABOUT_TEXT 4001

#define	CONFIG_DIALOG	6000
#define	CONFIG_HOST_FIELD	6001
#define	CONFIG_PORT_FIELD	6002
#define	CONFIG_EMPTY_BTN	6004
#define	CONFIG_REFRESH_BTN	6005
#define	CONFIG_USERNAME_FIELD	6006
#define	CONFIG_BITRATE_SELECT	6008
#define	CONFIG_PASSWORD_FIELD	6009
#define	CONFIG_UPGRADE_BTN	6012
#define	CONFIG_UPGRADE_CHECK	6013
#define	CONFIG_UPGRADE_TEXT	6014
#define	CONFIG_CONNECTED_TEXT	6015

#define	MAIN_DIALOG	8000
#define	MAIN_SEARCH_FIELD	8001
#define	MAIN_LIST	8002
#define	MAIN_CONFIG_BTN	8003
#define	MAIN_STATUS_TEXT	8004
#define	MAIN_CLEAR_BTN	8005
#define	MAIN_ENQUEUE_BTN	8007
#define	MAIN_ABORT_BTN	8008
#define	MAIN_PLAY_BTN	8009
#define	MAIN_REFRESH_BTN	8010

#define	AUTH_DIALOG	9000
#define	AUTH_USERNAME_FIELD	9001
#define	AUTH_PASSWORD_FIELD	9002
/*@ Prototypes @*/
#ifndef WEDIT_PROTOTYPES
#define WEDIT_PROTOTYPES
/*
 * Structure for dialog Dlg6000
 */
/*
 * Structure for dialog Dlg7000
 */
/*
 * Structure for dialog Dlg9000
 */
/*
 * Structure for dialog Dlg8000
 */
#endif
void SetDlgBkColor(HWND,COLORREF);
BOOL APIENTRY HandleCtlColor(UINT,DWORD);
/*
 * Callbacks for dialog Dlg6000
 */
/*
 * Callbacks for dialog Dlg7000
 */
/*
 * Callbacks for dialog Dlg9000
 */
/*
 * Callbacks for dialog Dlg8000
 */
extern void *GetDialogArguments(HWND);
extern char *GetDico(int,long);
/*@@ End Prototypes @@*/
