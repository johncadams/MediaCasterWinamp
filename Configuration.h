#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <string>
#include <windows.h>
#include "gen_ml/ml.h"

using namespace std;


class Configuration {
    private:        
        char   winampDir[1024];
        char   pluginDir[1024];
        char   iniPath  [1024];        
        char   host     [1024];
        char   port     [  16];
        char   user     [  32];
        char   pwrd     [  32];
        char   path     [1024];
        char   libr     [1024];
        char   logf     [1024];
        char   play     [1024];
        char   inst     [1024];
        char   bitr     [  32];
        char   msg      [1024];
        long   date;
        int    updt;
        int    logging;
        
        const char*        playlist;
        map<string,string> filters;
        map<string,int>    sortcols;
        map<string,int>    sortdirs;
        
        
    protected:
        void setWinampUserPassword();
        
        
    public:
/*      Configuration();
       ~Configuration();
*/        
        void load(winampMediaLibraryPlugin);
       
        const char* getHost         () { return host;      }
        const char* getPort         () { return port;      }       
        const char* getUser         () { return user;      }
        const char* getPassword     () { return pwrd;      }
        const char* getBitrate      () { return bitr;      }
        long        getBuildDate    () { return date;      }
        int         isAutoUpdate    () { return updt;      }         
        int         isLogging       () { return logging;   } 
        int         getSortColumn   () { return Configuration::sortcols[playlist]; }
        int         getSortDirection() { return Configuration::sortdirs[playlist]; }
        const char* getFilter       ();        
        const char* getMessage      () { return msg;       }
                 
        void        setHost         (const char*);
        void        setPort         (const char*);
        void        setUser         (const char*);
        void        setPassword     (const char*);
        void        setBitrate      (const char*);
        void        setBuildDate    (long);
        void        setAutoUpdate   (int);
        void        setLogging      (int);
        void        setPlaylist     (const char*);        
        void        setSortColumn   (int);
        void        reverseDirection();
        void        setFilter       (const char*);
        void        resetMessage    ();
        
        /* The following properties are read-only and hidden */
        const char* getLogfilePath  () { return logf;      } // filepath to ml_mcaster.log
        const char* getLibraryPath  () { return libr;      } // relative URL to library.txt
        const char* getPlaylistPath () { return play;      } // relative URL to playlists.txt
        const char* getInstallerPath() { return inst;      } // relative URL to MediaCaster.exe
        const char* getWinampDir    () { return winampDir; } // installation dir
        const char* getPluginDir    () { return pluginDir; } // Plugins dir
        string      getURL          (const char*);           // convienence URL builder (not streaming)
};

#endif /*CONFIG_H_*/
