/** @file
 *
 * VirtualBox External Authentication Library:
 * Linux PAM Authentication.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/* The PAM service name.
 *
 * The service name is the name of a file in the /etc/pam.d which contains
 * authentication rules. It is possible to use an existing service
 * name, like "login" for example. But if different set of rules
 * is required, one can create a new file /etc/pam.d/vrdpauth
 * specially for VRDP authentication. Note that the name of the
 * service must be lowercase. See PAM documentation for details.
 *
 * The Auth module takes the PAM service name from the
 * environment variable VBOX_AUTH_PAM_SERVICE. If the variable
 * is not specified, then the 'login' PAM service is used.
 */
#define VBOX_AUTH_PAM_SERVICE_NAME_ENV_OLD "VRDP_AUTH_PAM_SERVICE"
#define VBOX_AUTH_PAM_SERVICE_NAME_ENV "VBOX_AUTH_PAM_SERVICE"
#define VBOX_AUTH_PAM_DEFAULT_SERVICE_NAME "login"


/* The debug log file name.
 *
 * If defined, debug messages will be written to the file specified in the
 * VBOX_AUTH_DEBUG_FILENAME (or deprecated VRDP_AUTH_DEBUG_FILENAME) environment
 * variable:
 *
 * export VBOX_AUTH_DEBUG_FILENAME=pam.log
 *
 * The above will cause writing to the pam.log.
 */
#define VBOX_AUTH_DEBUG_FILENAME_ENV_OLD "VRDP_AUTH_DEBUG_FILENAME"
#define VBOX_AUTH_DEBUG_FILENAME_ENV "VBOX_AUTH_DEBUG_FILENAME"


/* Dynamic loading of the PAM library.
 *
 * If defined, the libpam.so is loaded dynamically.
 * Enabled by default since it is often required,
 * and does not harm.
 */
#define VBOX_AUTH_USE_PAM_DLLOAD


#ifdef VBOX_AUTH_USE_PAM_DLLOAD
/* The name of the PAM library */
# ifdef RT_OS_SOLARIS
#  define PAM_LIB_NAME "libpam.so.1"
# elif defined(RT_OS_FREEBSD)
#  define PAM_LIB_NAME "libpam.so"
# else
#  define PAM_LIB_NAME "libpam.so.0"
# endif
#endif /* VBOX_AUTH_USE_PAM_DLLOAD */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef RT_OS_FREEBSD
# include <malloc.h>
#endif

#include <security/pam_appl.h>

#include <VBox/VBoxAuth.h>

#ifdef VBOX_AUTH_USE_PAM_DLLOAD
#include <dlfcn.h>

static int (*fn_pam_start)(const char *service_name,
                           const char *user,
                           const struct pam_conv *pam_conversation,
                           pam_handle_t **pamh);
static int (*fn_pam_authenticate)(pam_handle_t *pamh, int flags);
static int (*fn_pam_acct_mgmt)(pam_handle_t *pamh, int flags);
static int (*fn_pam_end)(pam_handle_t *pamh, int pam_status);
static const char * (*fn_pam_strerror)(pam_handle_t *pamh, int errnum);
#else
#define fn_pam_start        pam_start
#define fn_pam_authenticate pam_authenticate
#define fn_pam_acct_mgmt    pam_acct_mgmt
#define fn_pam_end          pam_end
#define fn_pam_strerror     pam_strerror
#endif /* VBOX_AUTH_USE_PAM_DLLOAD */

static void debug_printf(const char *fmt, ...)
{
#if defined(VBOX_AUTH_DEBUG_FILENAME_ENV) || defined(VBOX_AUTH_DEBUG_FILENAME_ENV_OLD)
    va_list va;

    char buffer[1024];

    const char *filename = NULL;

    va_start(va, fmt);

#if defined(VBOX_AUTH_DEBUG_FILENAME_ENV)
    filename = getenv (VBOX_AUTH_DEBUG_FILENAME_ENV);
#endif /* VBOX_AUTH_DEBUG_FILENAME_ENV */

#if defined(VBOX_AUTH_DEBUG_FILENAME_ENV_OLD)
    if (filename == NULL)
    {
        filename = getenv (VBOX_AUTH_DEBUG_FILENAME_ENV_OLD);
    }
#endif /* VBOX_AUTH_DEBUG_FILENAME_ENV_OLD */

    if (filename)
    {
       FILE *f;

       vsnprintf (buffer, sizeof (buffer), fmt, va);

       f = fopen (filename, "ab");
       if (f != NULL)
       {
          fprintf (f, "%s", buffer);
          fclose (f);
       }
    }

    va_end (va);
#endif /* VBOX_AUTH_DEBUG_FILENAME_ENV || VBOX_AUTH_DEBUG_FILENAME_ENV_OLD */
}

#ifdef VBOX_AUTH_USE_PAM_DLLOAD

static void *gpvLibPam = NULL;

typedef struct _SymMap
{
    void **ppfn;
    const char *pszName;
} SymMap;

static SymMap symmap[] =
{
    { (void **)&fn_pam_start,        "pam_start" },
    { (void **)&fn_pam_authenticate, "pam_authenticate" },
    { (void **)&fn_pam_acct_mgmt,    "pam_acct_mgmt" },
    { (void **)&fn_pam_end,          "pam_end" },
    { (void **)&fn_pam_strerror,     "pam_strerror" },
    { NULL,                          NULL }
};

static int auth_pam_init(void)
{
    SymMap *iter;

    gpvLibPam = dlopen(PAM_LIB_NAME, RTLD_LAZY | RTLD_GLOBAL);

    if (!gpvLibPam)
    {
        debug_printf("auth_pam_init: dlopen %s failed\n", PAM_LIB_NAME);
        return PAM_SYSTEM_ERR;
    }

    iter = &symmap[0];

    while (iter->pszName != NULL)
    {
        void *pv = dlsym (gpvLibPam, iter->pszName);

        if (pv == NULL)
        {
            debug_printf("auth_pam_init: dlsym %s failed\n", iter->pszName);

            dlclose(gpvLibPam);
            gpvLibPam = NULL;

            return PAM_SYSTEM_ERR;
        }

        *iter->ppfn = pv;

        iter++;
    }

    return PAM_SUCCESS;
}

static void auth_pam_close(void)
{
    if (gpvLibPam)
    {
        dlclose(gpvLibPam);
        gpvLibPam = NULL;
    }

    return;
}
#else
static int auth_pam_init(void)
{
    return PAM_SUCCESS;
}

static void auth_pam_close(void)
{
    return;
}
#endif /* VBOX_AUTH_USE_PAM_DLLOAD */

static const char *auth_get_pam_service (void)
{
    const char *service = getenv (VBOX_AUTH_PAM_SERVICE_NAME_ENV);

    if (service == NULL)
    {
        service = getenv (VBOX_AUTH_PAM_SERVICE_NAME_ENV_OLD);

        if (service == NULL)
        {
            service = VBOX_AUTH_PAM_DEFAULT_SERVICE_NAME;
        }
    }

    debug_printf ("Using PAM service: %s\n", service);

    return service;
}

typedef struct _PamContext
{
    char *pszUser;
    char *pszPassword;
} PamContext;

#if defined(RT_OS_SOLARIS)
static int conv (int num_msg, struct pam_message **msg,
                 struct pam_response **resp, void *appdata_ptr)
#else
static int conv (int num_msg, const struct pam_message **msg,
                 struct pam_response **resp, void *appdata_ptr)
#endif
{
    int i;
    struct pam_response *r;

    PamContext *ctx = (PamContext *)appdata_ptr;

    if (ctx == NULL)
    {
        debug_printf("conv: ctx is NULL\n");
        return PAM_CONV_ERR;
    }

    debug_printf("conv: num %d u[%s] p[%d]\n", num_msg, ctx->pszUser, ctx->pszPassword? strlen (ctx->pszPassword): 0);

    r = (struct pam_response *) calloc (num_msg, sizeof (struct pam_response));

    if (r == NULL)
    {
        return PAM_CONV_ERR;
    }

    for (i = 0; i < num_msg; i++)
    {
        r[i].resp_retcode = 0;

        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
        {
            r[i].resp = strdup (ctx->pszPassword);
            debug_printf("conv: %d returning password [%d]\n", i, r[i].resp? strlen (r[i].resp): 0);
        }
        else if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON)
        {
            r[i].resp = strdup (ctx->pszUser);
            debug_printf("conv: %d returning name [%s]\n", i, r[i].resp);
        }
        else
        {
            debug_printf("conv: %d style %d: [%s]\n", i, msg[i]->msg_style, msg[i]->msg? msg[i]->msg: "(null)");
            r[i].resp = NULL;
        }
    }

    *resp = r;
    return PAM_SUCCESS;
}

/* The entry point must be visible. */
#if defined(_MSC_VER) || defined(__OS2__)
# define DECLEXPORT(type)       __declspec(dllexport) type
#else
# ifdef VBOX_HAVE_VISIBILITY_HIDDEN
#  define DECLEXPORT(type)      __attribute__((visibility("default"))) type
# else
#  define DECLEXPORT(type)      type
# endif
#endif

/* prototype to prevent gcc warning */
DECLEXPORT(AUTHENTRY3) AuthEntry;

DECLEXPORT(AuthResult) AUTHCALL AuthEntry(const char *pszCaller,
                                          PAUTHUUID pUuid,
                                          AuthGuestJudgement guestJudgement,
                                          const char *pszUser,
                                          const char *pszPassword,
                                          const char *pszDomain,
                                          int fLogon,
                                          unsigned clientId)
{
    AuthResult result = AuthResultAccessDenied;
    int rc;
    PamContext ctx;
    struct pam_conv pam_conversation;
    pam_handle_t *pam_handle = NULL;

    (void)pszCaller;
    (void)pUuid;
    (void)guestJudgement;
    (void)clientId;

    /* Only process logon requests. */
    if (!fLogon)
        return result; /* Return value is ignored by the caller. */

    debug_printf("u[%s], d[%s], p[%d]\n", pszUser, pszDomain, pszPassword ? strlen(pszPassword) : 0);

    ctx.pszUser     = (char *)pszUser;
    ctx.pszPassword = (char *)pszPassword;

    pam_conversation.conv        = conv;
    pam_conversation.appdata_ptr = &ctx;

    rc = auth_pam_init ();

    if (rc == PAM_SUCCESS)
    {
        debug_printf("init ok\n");

        rc = fn_pam_start(auth_get_pam_service (), pszUser, &pam_conversation, &pam_handle);

        if (rc == PAM_SUCCESS)
        {
            debug_printf("start ok\n");

            rc = fn_pam_authenticate(pam_handle, 0);

            if (rc == PAM_SUCCESS)
            {
                debug_printf("auth ok\n");

                rc = fn_pam_acct_mgmt(pam_handle, 0);
                if (rc == PAM_AUTHINFO_UNAVAIL
                    &&
                    getenv("VBOX_PAM_ALLOW_INACTIVE") != NULL)
                {
                    debug_printf("PAM_AUTHINFO_UNAVAIL\n");
                    rc = PAM_SUCCESS;
                }

                if (rc == PAM_SUCCESS)
                {
                    debug_printf("access granted\n");

                    result = AuthResultAccessGranted;
                }
                else
                {
                    debug_printf("pam_acct_mgmt failed %d. %s\n", rc, fn_pam_strerror (pam_handle, rc));
                }
            }
            else
            {
                debug_printf("pam_authenticate failed %d. %s\n", rc, fn_pam_strerror (pam_handle, rc));
            }

            fn_pam_end(pam_handle, rc);
        }
        else
        {
            debug_printf("pam_start failed %d\n", rc);
        }

        auth_pam_close ();

        debug_printf("auth_pam_close completed\n");
    }
    else
    {
        debug_printf("auth_pam_init failed %d\n", rc);
    }

    return result;
}

