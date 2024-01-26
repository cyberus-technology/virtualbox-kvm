

; Macro - Upgrade DLL File
; Written by Joost Verburg
; ------------------------
;
; Parameters:
; LOCALFILE   - Location of the new DLL file (on the compiler system)
; DESTFILE    - Location of the DLL file that should be upgraded
;              (on the user's system)
; TEMPBASEDIR - Directory on the user's system to store a temporary file
;               when the system has to be rebooted.
;               For Win9x support, this should be on the same volume as the
;               DESTFILE!
;               The Windows temp directory could be located on any volume,
;               so you cannot use  this directory.
;
; Define REPLACEDLL_NOREGISTER if you want to upgrade a DLL that does not
; have to be registered.
;
; Note: If you want to support Win9x, you can only use
;       short filenames (8.3).
;
; Example of usage:
; !insertmacro ReplaceDLL "dllname.dll" "$SYSDIR\dllname.dll" "$SYSDIR"
;

!macro ReplaceDLL LOCALFILE DESTFILE TEMPBASEDIR

  Push $R0
  Push $R1
  Push $R2
  Push $R3
  Push $R4
  Push $R5

  ;------------------------
  ;Unique number for labels

  !define REPLACEDLL_UNIQUE ${__LINE__}

  ;------------------------
  ;Copy the parameters used on run-time to a variable
  ;This allows the usage of variables as parameter

  StrCpy $R4 "${DESTFILE}"
  StrCpy $R5 "${TEMPBASEDIR}"

  ;------------------------
  ;Check file and version
  ;
  IfFileExists $R4 0 replacedll.copy_${REPLACEDLL_UNIQUE}

  ;ClearErrors
  ;  GetDLLVersionLocal "${LOCALFILE}" $R0 $R1
  ;  GetDLLVersion $R4 $R2 $R3
  ;IfErrors replacedll.upgrade_${REPLACEDLL_UNIQUE}
  ;
  ;IntCmpU $R0 $R2 0 replacedll.done_${REPLACEDLL_UNIQUE}
  ;  replacedll.upgrade_${REPLACEDLL_UNIQUE}
  ;IntCmpU $R1 $R3 replacedll.done_${REPLACEDLL_UNIQUE}
  ;  replacedll.done_${REPLACEDLL_UNIQUE}
  ;  replacedll.upgrade_${REPLACEDLL_UNIQUE}

  ;------------------------
  ;Let's replace the DLL!

  SetOverwrite try

  ;replacedll.upgrade_${REPLACEDLL_UNIQUE}:
    !ifndef REPLACEDLL_NOREGISTER
      ;Unregister the DLL
      UnRegDLL $R4
    !endif

  ;------------------------
  ;Try to copy the DLL directly

  ClearErrors
    StrCpy $R0 $R4
    Call :replacedll.file_${REPLACEDLL_UNIQUE}
  IfErrors 0 replacedll.noreboot_${REPLACEDLL_UNIQUE}

  ;------------------------
  ;DLL is in use. Copy it to a temp file and Rename it on reboot.

  GetTempFileName $R0 $R5
    Call :replacedll.file_${REPLACEDLL_UNIQUE}
  Rename /REBOOTOK $R0 $R4

  ;------------------------
  ;Register the DLL on reboot

  !ifndef REPLACEDLL_NOREGISTER
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\RunOnce" \
      "Register $R4" 'rundll32.exe "$R4",DllRegisterServer'
  !endif

  Goto replacedll.done_${REPLACEDLL_UNIQUE}

  ;------------------------
  ;DLL does not exist - just extract

  replacedll.copy_${REPLACEDLL_UNIQUE}:
    StrCpy $R0 $R4
    Call :replacedll.file_${REPLACEDLL_UNIQUE}

  ;------------------------
  ;Register the DLL

  replacedll.noreboot_${REPLACEDLL_UNIQUE}:
    !ifndef REPLACEDLL_NOREGISTER
      RegDLL $R4
    !endif

  ;------------------------
  ;Done

  replacedll.done_${REPLACEDLL_UNIQUE}:

  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0

  ;------------------------
  ;End

  Goto replacedll.end_${REPLACEDLL_UNIQUE}

  ;------------------------
  ;Called to extract the DLL

  replacedll.file_${REPLACEDLL_UNIQUE}:
    File /oname=$R0 "${LOCALFILE}"
    Return

  replacedll.end_${REPLACEDLL_UNIQUE}:

 ;------------------------
 ;Restore settings

 SetOverwrite lastused

 !undef REPLACEDLL_UNIQUE

!macroend
