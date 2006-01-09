
#include <windows.h>

#include "Process.h"
#include "Trace.h"


static int execProcessAndWait(const char* cmdline, int wait) {
    TRACE("execProcessAndWait");
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    int rtn = CreateProcess(NULL,                   // No module name (use command line). 
                            TEXT((char*)cmdline),   // Command line. 
                            NULL,                   // Process handle not inheritable. 
                            NULL,                   // Thread handle not inheritable. 
                            FALSE,                  // Set handle inheritance to FALSE. 
                            0,                      // No creation flags. 
                            NULL,                   // Use parent's environment block. 
                            NULL,                   // Use parent's starting directory. 
                            &si,                    // Pointer to STARTUPINFO structure.
                            &pi);                   // Pointer to PROCESS_INFORMATION structure.
                            
    if (!wait || rtn==0) return rtn;
       
    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    
    return 42;
}


int execForegroundProcess(const char* cmdline) {
    TRACE("execForegroundProcess");
    LOGGER("Executing", cmdline);
    return execProcessAndWait(cmdline, 0);
}

   
int execBackgroundProcess(const char* cmdline) {
    TRACE("execBackgroundProcess");
    LOGGER("Executing", cmdline);
    return execProcessAndWait(cmdline, 1);
}
