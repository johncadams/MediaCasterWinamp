#ifndef UPGRADE_H_
#define UPGRADE_H_

#include "HTTPGet.h"


class Upgrade {
    private:
        HWND   hwnd;
        int    connectionProblem;
        string installerUrl;
        
    protected:
        void downloadFunction()     throw(ConnectionException);
        
    public:     
        Upgrade();
          
        void        download()      throw(ConnectionException);
        int         isAvailable()   throw(ConnectionException);
        const char* getIsAvailableStatus();
        void        setHwnd(HWND);
};

#endif /*UPGRADE_H_*/
