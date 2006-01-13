#ifndef UPGRADE_H_
#define UPGRADE_H_

#include "HTTPGet.h"


class Upgrade {
    private:
        HWND hwnd;
        
    protected:
        void downloadFunction()     throw(ConnectionException);
        
    public:       
        void download        ()     throw(ConnectionException);
        int  isAvailable     ()     throw(ConnectionException);
        void setHwnd         (HWND);
};

#endif /*UPGRADE_H_*/
