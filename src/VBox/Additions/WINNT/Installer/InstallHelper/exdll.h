/*
 * Copyright (C) 1995-2009 Contributors
 * More detailed copyright information can be found in the individual source code files.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/** Taken from:
 *  http://nsis.sourceforge.net/Examples/Plugin/exdll.h
 */

#ifndef GA_INCLUDED_SRC_WINNT_Installer_InstallHelper_exdll_h
#define GA_INCLUDED_SRC_WINNT_Installer_InstallHelper_exdll_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/win/windows.h>

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

// Starting with NSIS 2.42, you can check the version of the plugin API in exec_flags->plugin_api_version
// The format is 0xXXXXYYYY where X is the major version and Y is the minor version (MAKELONG(y,x))
// When doing version checks, always remember to use >=, ex: if (pX->exec_flags->plugin_api_version >= NSISPIAPIVER_1_0) {}

#define NSISPIAPIVER_1_0 0x00010000
#define NSISPIAPIVER_CURR NSISPIAPIVER_1_0

// NSIS Plug-In Callback Messages
enum NSPIM
{
    NSPIM_UNLOAD,    // This is the last message a plugin gets, do final cleanup
    NSPIM_GUIUNLOAD, // Called after .onGUIEnd
};

/** Defines the maximum string length NSIS can handle.
 *  Note: This depends on the NSIS build being used, e.g. there are different builds which can also handle larger strings.
 *        So to play safe go with the minimum (default) string length here. */
#define NSIS_MAX_STRLEN 1024

// Prototype for callbacks registered with extra_parameters->RegisterPluginCallback()
// Return NULL for unknown messages
// Should always be __cdecl for future expansion possibilities
typedef UINT_PTR (*NSISPLUGINCALLBACK)(enum NSPIM);

#ifndef NSISCALL
#  define NSISCALL __stdcall
#endif

#ifndef VBOX
// only include this file from one place in your DLL.
// (it is all static, if you use it in two places it will fail)

#define EXDLL_INIT()           {  \
        g_stringsize=string_size; \
        g_stacktop=stacktop;      \
        g_variables=variables; }
#endif /* !VBOX */

typedef struct _stack_t
{
    struct _stack_t *next;
    TCHAR text[1]; // this should be the length of string_size
} stack_t;

// extra_parameters data structures containing other interesting stuff
// but the stack, variables and HWND passed on to plug-ins.
typedef struct
{
    int autoclose;
    int all_user_var;
    int exec_error;
    int abort;
    int exec_reboot; // NSIS_SUPPORT_REBOOT
    int reboot_called; // NSIS_SUPPORT_REBOOT
    int XXX_cur_insttype; // depreacted
    int plugin_api_version; // see NSISPIAPIVER_CURR
                          // used to be XXX_insttype_changed
    int silent; // NSIS_CONFIG_SILENT_SUPPORT
    int instdir_error;
    int rtl;
    int errlvl;
    int alter_reg_view;
    int status_update;
} exec_flags_t;

typedef struct
{
    exec_flags_t *exec_flags;
    int (NSISCALL *ExecuteCodeSegment)(int, HWND);
    void (NSISCALL *validate_filename)(TCHAR *);
    BOOL (NSISCALL *RegisterPluginCallback)(HMODULE, NSISPLUGINCALLBACK);
} extra_parameters;

#ifndef VBOX
static unsigned int g_stringsize;
static stack_t **g_stacktop;
static TCHAR *g_variables;
#endif

enum
{
INST_0,         // $0
INST_1,         // $1
INST_2,         // $2
INST_3,         // $3
INST_4,         // $4
INST_5,         // $5
INST_6,         // $6
INST_7,         // $7
INST_8,         // $8
INST_9,         // $9
INST_R0,        // $R0
INST_R1,        // $R1
INST_R2,        // $R2
INST_R3,        // $R3
INST_R4,        // $R4
INST_R5,        // $R5
INST_R6,        // $R6
INST_R7,        // $R7
INST_R8,        // $R8
INST_R9,        // $R9
INST_CMDLINE,   // $CMDLINE
INST_INSTDIR,   // $INSTDIR
INST_OUTDIR,    // $OUTDIR
INST_EXEDIR,    // $EXEDIR
INST_LANG,      // $LANGUAGE
__INST_LAST
};

#ifndef VBOX

// utility functions (not required but often useful)
int popstringn(TCHAR *str, int maxlen)
{
  stack_t *th;
  if (!g_stacktop || !*g_stacktop) return 1;
  th=(*g_stacktop);
  if (str) lstrcpyn(str,th->text,maxlen?maxlen:g_stringsize);
  *g_stacktop = th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}

static void __stdcall pushstring(const TCHAR *str)
{
  stack_t *th;
  if (!g_stacktop)
      return;
  th=(stack_t*)GlobalAlloc(GPTR,sizeof(stack_t)+g_stringsize);
  lstrcpyn(th->text,str,g_stringsize);
  th->next=*g_stacktop;
  *g_stacktop=th;
}

static TCHAR* __stdcall getuservariable(const int varnum)
{
  if (varnum < 0 || varnum >= __INST_LAST)
      return NULL;
  return g_variables+varnum*g_stringsize;
}

static void __stdcall setuservariable(const int varnum, const TCHAR *var)
{
    if (var != NULL && varnum >= 0 && varnum < __INST_LAST)
        lstrcpy(g_variables + varnum*g_stringsize, var);
}

#endif /* ! VBOX */

#endif /* !GA_INCLUDED_SRC_WINNT_Installer_InstallHelper_exdll_h */
