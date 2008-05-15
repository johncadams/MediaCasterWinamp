; MediaCaster.nsi
;
; This script will generate an installer that installs a Winamp 2 plug-in.
;
; This installer will automatically alert the user that installation was
; successful, and ask them whether or not they would like to make the 
; plug-in the default and run Winamp.

;--------------------------------

; Uncomment the next line to enable auto Winamp download
; !define WINAMP_AUTOINSTALL

!include WinMessages.nsh

!ifndef DIR
!define DIR "Build"
!endif

; The name of the installer
Name "Media Caster ver ${MC_VERSION}"

; The file to write
OutFile ${DIR}\MediaCaster.exe

; The default installation directory
InstallDir $PROGRAMFILES\Winamp

; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

; The text to prompt the user to enter a directory
DirText "Please select your Winamp path below (you will be able to proceed when Winamp is detected):"
# currently doesn't work - DirShow hide

; automatically close the installer when done.
AutoCloseWindow true

; hide the "show details" box
ShowInstDetails nevershow

; location of Winamp for auto-installing
!define winampInstaller "http://download.nullsoft.com/winamp/client/winamp521_full_bundle_emusic-7plus.exe"

;--------------------------------

;Pages

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install

Section ""

!ifdef WINAMP_AUTOINSTALL
  Call MakeSureIGotWinamp
!endif

  Call QueryWinampVisPath
  SetOutPath $1
  
  ; Close Winamp if its running
  stopWinamp:
    FindWindow $0 "Winamp v1.x"
    IntCmp $0 0 stopWinampDone
    SendMessage $0 ${WM_CLOSE} 0 0
    Sleep 5000		; hopefully long enough, really need a better test
    Goto stopWinamp
  stopWinampDone:
  

  ; installDll:
    ; File to extract
    ClearErrors
    Delete $INSTDIR\Plugins\ml_mcaster.dll
    IfErrors 0 +3
      WriteINIStr $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster message "Cannot replace $INSTDIR\Plugins\ml_mcaster.dll"
      goto startWinamp      
    File ${DIR}\ml_mcaster.dll
  
  
  ; createRegistry:
    ; Write the installation path into the registry
    WriteRegStr   HKLM SOFTWARE\MediaCaster "Install_Dir" "$INSTDIR"
  
    ; Write the uninstall keys for Windows
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster" "DisplayName" "MediaCaster (v${MC_VERSION}) for Winamp"
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster" "UninstallString" '"$INSTDIR\mcaster_uninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster" "NoRepair" 1
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster" "Version" "${MC_VERSION}"
    WriteUninstaller "$INSTDIR\mcaster_uninstall.exe"
  
  
  ; createIni:
    ; Delete anything from the original alpha
    Delete       $INSTDIR\Plugins\ml_caster.dll
    DeleteINISec $INSTDIR\winamp.ini ml_caster
  
    ; No longer used starting w/1.0
    DeleteINIStr $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster enabled
    DeleteINIStr $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster lastfilter
    
    ; Clear the error message
    DeleteINIStr $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster message
    
    ; Create the cache directory
    CreateDirectory $INSTDIR\Plugins\ml_mcaster_cache
    
    ; reset the timestamp from newtimestamp
    ReadINIStr $0 $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster newtimestamp
    StrCmp $0 "" startWinamp
    WriteINIStr   $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster timestamp $0
    DeleteINIStr  $INSTDIR\Plugins\ml_mcaster.ini ml_mcaster newtimestamp
  
  startWinamp:
    ; prompt user, and if they select yes run winamp
    MessageBox MB_YESNO|MB_ICONQUESTION \
             "The plug-in was installed. Would you like to run Winamp now?" \
             /SD IDYES \
             IDNO Done
    Exec '"$INSTDIR\Winamp.exe"'
    
  Done:
 
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MediaCaster"
  DeleteRegKey HKLM SOFTWARE\MediaCaster
  
  ; Remove the files and uninstaller
  Delete $INSTDIR\Plugins\ml_mcaster.dll
  Delete $INSTDIR\Plugins\ml_mcaster.ini
  RMDir  /r $INSTDIR\Plugins\ml_mcaster_cache
  Delete $INSTDIR\mcaster_uninstall.exe
  Delete $INSTDIR\ml_mcaster.log   ; this path can be overridden in the ini 
  
  ; Delete the <= 1.1 trace log
  Delete C:\ml_mcaster.log

SectionEnd

;--------------------------------

Function .onVerifyInstDir

!ifndef WINAMP_AUTOINSTALL

  ;Check for Winamp installation

  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:

!endif ; WINAMP_AUTOINSTALL

FunctionEnd

;--------------------------------

Function QueryWinampVisPath ; sets $1 with vis path

  StrCpy $1 $INSTDIR\Plugins
  ; use DSPDir instead of VISDir to get DSP plugins directory
  ReadINIStr $9 $INSTDIR\winamp.ini Winamp VisDir 
  StrCmp $9 "" End
  IfFileExists $9 0 End
    StrCpy $1 $9 ; update dir
  End:
  
FunctionEnd

;--------------------------------

!ifdef WINAMP_AUTOINSTALL

Function GetWinampInstPath

  Push $0
  Push $1
  Push $2
  ReadRegStr $0 HKLM \
     "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \ 
     "UninstallString"
  StrCmp $0 "" fin

    StrCpy $1 $0 1 0 ; get firstchar
    StrCmp $1 '"' "" getparent 
      ; if first char is ", let's remove "'s first.
      StrCpy $0 $0 "" 1
      StrCpy $1 0
      rqloop:
        StrCpy $2 $0 1 $1
        StrCmp $2 '"' rqdone
        StrCmp $2 "" rqdone
        IntOp $1 $1 + 1
        Goto rqloop
      rqdone:
      StrCpy $0 $0 $1
    getparent:
    ; the uninstall string goes to an EXE, let's get the directory.
    StrCpy $1 -1
    gploop:
      StrCpy $2 $0 1 $1
      StrCmp $2 "" gpexit
      StrCmp $2 "\" gpexit
      IntOp $1 $1 - 1
      Goto gploop
    gpexit:
    StrCpy $0 $0 $1

    StrCmp $0 "" fin
    IfFileExists $0\winamp.exe fin
      StrCpy $0 ""
  fin:
  Pop $2
  Pop $1
  Exch $0
  
FunctionEnd

;--------------------------------

Function MakeSureIGotWinamp

  Call GetWinampInstPath
  
  Pop $0
  StrCmp $0 "" getwinamp
    Return
    
  getwinamp:
  
  Call ConnectInternet ;Make an internet connection (if no connection available)
  
  StrCpy $2 "$TEMP\Winamp Installer.exe"
  NSISdl::download winampInstaller $2
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint "download failed: $0"
    Abort
  success:
    ExecWait '"$2" /S'
    Delete $2
    Call GetWinampInstPath
    Pop $0
    StrCmp $0 "" skip
    StrCpy $INSTDIR $0
  skip:
  
FunctionEnd

;--------------------------------

Function ConnectInternet

  Push $R0
    
    ClearErrors
    Dialer::AttemptConnect
    IfErrors noie3
    
    Pop $R0
    StrCmp $R0 "online" connected
      MessageBox MB_OK|MB_ICONSTOP "Cannot connect to the internet."
      Quit
    
    noie3:
  
    ; IE3 not installed
    MessageBox MB_OK|MB_ICONINFORMATION "Please connect to the internet now."
    
    connected:
  
  Pop $R0
  
FunctionEnd

!endif ; WINAMP_AUTOINSTALL
