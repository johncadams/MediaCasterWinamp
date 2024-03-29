
#include <windows.h>
#include <stdio.h>

#include "winamp/wa_dlg.h"
#include "MediaCaster.h"
#include "Configuration.h"
#include "Trace.h"


// configuration section in winamp.ini
#define CONFIG_SEC          "ml_mcaster"
#define HOST_PROPERTY       "host"
#define PORT_PROPERTY       "port"
#define USER_PROPERTY       "user"
#define PWRD_PROPERTY       "pwrd"
#define BITR_PROPERTY       "bitrate"
#define UDIR_PROPERTY       "path"
#define LIBR_PROPERTY       "library"
#define LOGF_PROPERTY       "logfile"
#define LOGGING_PROPERTY    "logging"
#define THREADS_PROPERTY	"threads"
#define PLAY_PROPERTY       "playlists"
#define INST_PROPERTY       "installer"
#define FILTER_PROPERTY     "lastfilter"
#define SORTCOL_PROPERTY    "sortcol"
#define SORTDIR_PROPERTY    "sortdir"
#define DATE_PROPERTY       "timestamp"
#define TMPDATE_PROPERTY	"newtimestamp"
#define UPDATE_PROPERTY     "autoupdate"
#define MESG_PROPERTY       "message"
#define SDIR_PROPERTY       "saveDir"

// Default values
#define DEFAULT_HOST        "mcaster.kicks-ass.net"
#define DEFAULT_PORT        "9000" // 80 when blank
#define DEFAULT_USER        ""
#define DEFAULT_PWRD        ""
#define DEFAULT_UDIR        "/mcaster/"
#ifdef IS_BETA
#define DEFAULT_LOGGING		1
#else
#define DEFAULT_LOGGING		0
#endif
#define DEFAULT_LOGF		"ml_mcaster.log"
#define DEFAULT_LIBR        "library.txt"  // These values can be absolute
#define DEFAULT_PLAY        "playlists.txt"
#ifdef IS_BETA
#define DEFAULT_INST        "Releases/MediaCasterLatest.exe"
#else
#define DEFAULT_INST        "Releases/MediaCaster.exe"
#endif
#define DEFAULT_BITR        "stream=1;bitrate=56%20kbps"
#define DEFAULT_DATE		0
#define DEFAULT_UPDATE      1
#define DEFAULT_MESG        ""
#define DEFAULT_CACHE		"ml_mcaster_cache\\"
#define DEFAULT_THREADS		1
#define DEFAULT_SDIR        ""

/*
Configuration::Configuration() {
    TRACE("Configuration::Configuration");
}

Configuration::~Configuration() {
    TRACE("Configuration::~Configuration");
}
*/


void Configuration::init(const char* rootDir) {
	TRACE("Configuration::load");
    strcpy(winampDir, rootDir);
    strcat(winampDir, "\\");
    sprintf(pluginDir, "%sPlugins\\",  winampDir);
    sprintf(iniPath,   "%s%s.ini", pluginDir, CONFIG_SEC);
        
	logging = GetPrivateProfileInt(CONFIG_SEC,LOGGING_PROPERTY, DEFAULT_LOGGING,iniPath);
	threads = GetPrivateProfileInt(CONFIG_SEC,THREADS_PROPERTY, DEFAULT_THREADS,iniPath);
    updt    = GetPrivateProfileInt(CONFIG_SEC,UPDATE_PROPERTY,  DEFAULT_UPDATE, iniPath);
    
    GetPrivateProfileString(CONFIG_SEC,HOST_PROPERTY,  DEFAULT_HOST, host,sizeof(host),iniPath);
    GetPrivateProfileString(CONFIG_SEC,PORT_PROPERTY,  DEFAULT_PORT, port,sizeof(port),iniPath);
    GetPrivateProfileString(CONFIG_SEC,USER_PROPERTY,  DEFAULT_USER, user,sizeof(user),iniPath);
    GetPrivateProfileString(CONFIG_SEC,PWRD_PROPERTY,  DEFAULT_PWRD, pwrd,sizeof(pwrd),iniPath);
    GetPrivateProfileString(CONFIG_SEC,UDIR_PROPERTY,  DEFAULT_UDIR, udir,sizeof(udir),iniPath);
    GetPrivateProfileString(CONFIG_SEC,LOGF_PROPERTY,  DEFAULT_LOGF, logf,sizeof(logf),iniPath);
    GetPrivateProfileString(CONFIG_SEC,LIBR_PROPERTY,  DEFAULT_LIBR, libr,sizeof(libr),iniPath);
    GetPrivateProfileString(CONFIG_SEC,PLAY_PROPERTY,  DEFAULT_PLAY, play,sizeof(play),iniPath);
    GetPrivateProfileString(CONFIG_SEC,INST_PROPERTY,  DEFAULT_INST, inst,sizeof(inst),iniPath);
    GetPrivateProfileString(CONFIG_SEC,BITR_PROPERTY,  DEFAULT_BITR, bitr,sizeof(bitr),iniPath);
    GetPrivateProfileString(CONFIG_SEC,MESG_PROPERTY,  DEFAULT_MESG, mesg,sizeof(mesg),iniPath);
    GetPrivateProfileString(CONFIG_SEC,SDIR_PROPERTY,  DEFAULT_SDIR, sdir,sizeof(sdir),iniPath);
    
    // This forces things to be written out as to avoid having default values hidden
    Configuration::setWinampUserPassword();
    Configuration::setLogging(logging);
    Configuration::setThreaded(threads);
    
    if (logf[0]!='/') {
        char tmp[1024];
        sprintf(tmp, "%s\\%s", winampDir, logf);
        strcpy(logf, tmp);
    }
    
    playlist = PLUGIN_NAME;
    
    // Get the playlist specific resources
    char filter[1024];
    GetPrivateProfileString(playlist, FILTER_PROPERTY, "", filter, sizeof(filter), iniPath);
    filters [playlist] = strdup(filter);
    sortcols[playlist] = GetPrivateProfileInt(playlist, SORTCOL_PROPERTY, 0, iniPath);
    sortdirs[playlist] = GetPrivateProfileInt(playlist, SORTDIR_PROPERTY, 0, iniPath);
}

void Configuration::init(winampMediaLibraryPlugin plugin) {
    TRACE("Configuration::load");
    char winampDir[1024];
    strcpy(winampDir, (char*)SendMessage(plugin.hwndWinampParent,WM_WA_IPC,0,IPC_GETINIDIRECTORY));
    init(winampDir);
}


string Configuration::getURL(const char* file) const {
    TRACE("Configuration::getURL");
    string port = Configuration::getPort()[0]?Configuration::getPort():"80";
    string base = string("http://") +Configuration::getHost() +":" +port;
    if (file[0]!='/') base += udir;
    base += file;
    LOGGER("URL", base.c_str());
    return base;
}


string Configuration::getCacheDir() const {
	TRACE("Configuration::getCacheDir");
	string pluginDir = Configuration::getPluginDir();
    pluginDir += DEFAULT_CACHE;
    return pluginDir;
}


string Configuration::getCacheFile(const char* file) const {
    TRACE("Configuration::getCacheFile");
    string cacheDir = Configuration::getCacheDir();
    cacheDir += file; // These files (as currently being used) look absolute but are relative
    LOGGER("FILE", cacheDir.c_str());
    return cacheDir;
}


void Configuration::setPlaylist(const char* playlist) {
    TRACE("Configuration::setPlaylist");
    Configuration::playlist = playlist;
}


void Configuration::setFilter(const char* filter) {
    TRACE("Configuration::setFilter");
    Configuration::filters.erase(playlist);
    Configuration::filters[playlist] = filter;
    WritePrivateProfileString(playlist, FILTER_PROPERTY, filter, iniPath);
}


void Configuration::reverseDirection() {
    TRACE("Configuration::reverseDirection");
    int sortdir = Configuration::sortdirs[playlist];
    Configuration::sortdirs[playlist] = sortdir?1:0;
    WritePrivateProfileString(playlist, SORTDIR_PROPERTY, sortdir?"1":"0", iniPath);
}


void Configuration::setSortColumn(int column) {
    TRACE("Configuration::setSortColumn");
    Configuration::sortcols[playlist] = column;
    
    char tmp[16];
    sprintf(tmp, "%d", column);
    WritePrivateProfileString(playlist, SORTCOL_PROPERTY, tmp, iniPath);
}


void Configuration::setHost(const char* host) {
    TRACE("Configuration::setHost");
    strcpy(Configuration::host, host);
    WritePrivateProfileString(CONFIG_SEC, HOST_PROPERTY, host, iniPath);
}


void Configuration::setPort(const char* port) {
    TRACE("Configuration::setPort");
    strcpy(Configuration::port, port);
    WritePrivateProfileString(CONFIG_SEC, PORT_PROPERTY, port, iniPath);    
}


void Configuration::setUsername(const char* user) {
    TRACE("Configuration::setUsername");
    strcpy(Configuration::user, user);
    WritePrivateProfileString(CONFIG_SEC, USER_PROPERTY, user, iniPath);
    setWinampUserPassword();
}


void Configuration::setPassword(const char* password) {
    TRACE("Configuration::setPassword");
    strcpy(Configuration::pwrd, password);
    WritePrivateProfileString(CONFIG_SEC, PWRD_PROPERTY, pwrd, iniPath);
    setWinampUserPassword();
}

void Configuration::setBitrate(const char* bitrate) {
    TRACE("Configuration::setBitrate");
    strcpy(Configuration::bitr, bitrate);
    WritePrivateProfileString(CONFIG_SEC, BITR_PROPERTY, bitr, iniPath);
}


void Configuration::setAutoUpdate(int updt) {
    TRACE("Configuration::setAutoUpdate");
    Configuration::updt = updt;
    WritePrivateProfileString(CONFIG_SEC, UPDATE_PROPERTY, updt?"1":"0", iniPath);
}


void Configuration::setLogging(int logging) {
    TRACE("Configuration::setLogging");
    Configuration::logging = logging;
    WritePrivateProfileString(CONFIG_SEC, LOGGING_PROPERTY, logging?"1":"0", iniPath);
}


void Configuration::setThreaded(int threads) {
    TRACE("Configuration::setThreaded");
    Configuration::threads = threads;
    WritePrivateProfileString(CONFIG_SEC, THREADS_PROPERTY, threads?"1":"0", iniPath);
}


void Configuration::resetMessage() {
    TRACE("Configuration::resetMessage");
    strcpy(Configuration::mesg, "");
    WritePrivateProfileString(CONFIG_SEC, MESG_PROPERTY, mesg, iniPath);
}


void Configuration::setWinampUserPassword() {
    TRACE("Configuration::setWinampUserPassword");
    char iniPath[1024];
    char userPass[128];
    sprintf(iniPath, "%s\\\\%s.ini", winampDir, "Winamp");
    sprintf(userPass, "%s:%s", user, pwrd);
    
    // "Media Caster" is the name of the HTTP Realm, we should extract this from the request
    // We also need a better way of interacing with this property should Winamp change it
    WritePrivateProfileString("HTTP-AUTH", "Media Caster", userPass, iniPath);
}


void Configuration::setSaveDir(const char* saveDir) {
	TRACE("Configuration::setSaveDir");
	strcpy(Configuration::sdir, saveDir);
    WritePrivateProfileString(CONFIG_SEC, SDIR_PROPERTY, saveDir, iniPath);
}
