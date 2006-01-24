#ifndef MESSAGES_H_
#define MESSAGES_H_

#include "MediaCaster.h"
                          

/* General error/status messages */
#define CONN_PROBLEM_STATUS1    "Connection FAILURE"
#define CONN_PROBLEM_MSGBOX     "There was a problem connecting to the Media Caster server\n "\
                                "\n"\
                                "Cause: %s"
                                
#define COPYRIGHT               "Brought to you by the fine folks at DigitalStreams,\n"  \
                                "a wholly owned subsidary of Smada Nhoj Industries.\n"   \
                                "Copyright 2005, all rights reserved"
                                
#define CONTACT_US              "Contact DigitalStreams,\n"\
                                "a wholly owned subsidary of Smada Nhoj Industries."
                                
#define INSTALLER_INSTRUCTS     "The auto-installer failed, you will need to run the installer manully:\n"\
                                "   Cause: %s\n"\
                                "\n"\
                                "Workaround:\n"\
                                "   1) Terminate Winamp\n"\
                                "   2) Download the installer (%s)\n"\
                                "   3) Execute the installer\n"\
                                "   4) Re-enable Auto-Update (Config dialog)"

        
/* Used in the status bar */
#define CONNECTING              "[Connecting...]"
#define CONNECTED_UPDATING      "[Connected] Checking for updates"
#define DISPLIST_DOWNLOAD       "[Connected] Retrieving list: %3d%%"
#define INSTALLER_DOWNLOAD      "[Connected] Downloading installer: %3d%%"

#define NUMITEMS_NODAYS         "%d items [%d:%02d:%02d]"
#define NUMITEMS_WDAYS          "%d items [%d days+%d:%02d:%02d]"


/* The following are used for upgrades in the status bar and config dialog box */
#define CONN_PROBLEM_STATUS2    "Connection error, status unavailable"
#define UPGRADE_AVAIL_STATUS    "Media Caster upgrade available"
#define UPTODATE_STATUS         "Media Caster is up-to-date"
#define UPGRADE_AVAIL_MSGBOX    "A newer version of "PLUGIN_NAME" is available\n"\
                                "\n"\
                                "Would you like to upgrade?"


/* The following are used in downloading playlists */
#define UNKNOWN_PLAYLIST_TYPE   "An unknown playlist type was found."


/* Tracer messages */
#define ERROR_OPENING_LOGFILE   "Error opening log file"
#define COPYRIGHT_MSGBOX        PLUGIN_FULLNAME "\n \n" COPYRIGHT


#endif /*MESSAGES_H_*/