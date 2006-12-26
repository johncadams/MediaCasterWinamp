#ifndef UPGRADE_H_
#define UPGRADE_H_

#include "Http.h"


class Upgrade {
    private:
        HWND   hwnd;
        int    connProblem;
        string installerUrl;
        
    protected:
        void       downloadFunction()      throw(ConnectionException);
        
    public:     
        Upgrade();
          
        void        download()             throw(ConnectionException);
        int         isAvailable()          throw(ConnectionException);
        const char* getIsAvailableStatus() throw();
        void        setHwnd(HWND);
};

#endif /*UPGRADE_H_*/
