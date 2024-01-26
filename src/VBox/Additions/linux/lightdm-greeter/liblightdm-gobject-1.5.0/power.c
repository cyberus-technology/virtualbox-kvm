/*
 * Copyright (C) 2010-2011 Robert Ancell.
 * Author: Robert Ancell <robert.ancell@canonical.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include "config.h"

#include <string.h>
#include <gio/gio.h>

#include "lightdm/power.h"

static GDBusProxy *upower_proxy = NULL;
static GDBusProxy *ck_proxy = NULL;

static gboolean
upower_call_function (const gchar *function, gboolean default_result, GError **error)
{
    GVariant *result;
    gboolean function_result = FALSE;

    if (!upower_proxy)
    {
        upower_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      NULL,
                                                      "org.freedesktop.UPower",
                                                      "/org/freedesktop/UPower",
                                                      "org.freedesktop.UPower",
                                                      NULL,
                                                      error);
        if (!upower_proxy)
            return FALSE;
    }

    result = g_dbus_proxy_call_sync (upower_proxy,
                                     function,
                                     NULL,
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     error);
    if (!result)
        return default_result;

    if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(b)")))
        g_variant_get (result, "(b)", &function_result);

    g_variant_unref (result);
    return function_result;
}

/**
 * lightdm_get_can_suspend:
 *
 * Checks if authorized to do a system suspend.
 *
 * Return value: #TRUE if can suspend the system
 **/
gboolean
lightdm_get_can_suspend (void)
{
    return upower_call_function ("SuspendAllowed", FALSE, NULL);
}

/**
 * lightdm_suspend:
 * @error: return location for a #GError, or %NULL
 *
 * Triggers a system suspend.
 * 
 * Return value: #TRUE if suspend initiated.
 **/
gboolean
lightdm_suspend (GError **error)
{
    return upower_call_function ("Suspend", TRUE, error);
}

/**
 * lightdm_get_can_hibernate:
 *
 * Checks if is authorized to do a system hibernate.
 *
 * Return value: #TRUE if can hibernate the system
 **/
gboolean
lightdm_get_can_hibernate (void)
{
    return upower_call_function ("HibernateAllowed", FALSE, NULL);
}

/**
 * lightdm_hibernate:
 * @error: return location for a #GError, or %NULL
 *
 * Triggers a system hibernate.
 * 
 * Return value: #TRUE if hibernate initiated.
 **/
gboolean
lightdm_hibernate (GError **error)
{
    return upower_call_function ("Hibernate", TRUE, error);
}

static gboolean
ck_call_function (const gchar *function, gboolean default_result, GError **error)
{
    GVariant *result;
    gboolean function_result = FALSE;

    if (!ck_proxy)
    {
        ck_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  NULL,
                                                  "org.freedesktop.ConsoleKit",
                                                  "/org/freedesktop/ConsoleKit/Manager",
                                                  "org.freedesktop.ConsoleKit.Manager",
                                                  NULL,
                                                  error);
        if (!ck_proxy)
            return FALSE;
    }

    result = g_dbus_proxy_call_sync (ck_proxy,
                                     function,
                                     NULL,
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1,
                                     NULL,
                                     error);

    if (!result)
        return default_result;

    if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(b)")))
        g_variant_get (result, "(b)", &function_result);

    g_variant_unref (result);
    return function_result;
}

/**
 * lightdm_get_can_restart:
 *
 * Checks if is authorized to do a system restart.
 *
 * Return value: #TRUE if can restart the system
 **/
gboolean
lightdm_get_can_restart (void)
{
    return ck_call_function ("CanRestart", FALSE, NULL);
}

/**
 * lightdm_restart:
 * @error: return location for a #GError, or %NULL
 *
 * Triggers a system restart.
 *
 * Return value: #TRUE if restart initiated.
 **/
gboolean
lightdm_restart (GError **error)
{
    return ck_call_function ("Restart", TRUE, error);
}

/**
 * lightdm_get_can_shutdown:
 *
 * Checks if is authorized to do a system shutdown.
 *
 * Return value: #TRUE if can shutdown the system
 **/
gboolean
lightdm_get_can_shutdown (void)
{
    return ck_call_function ("CanStop", FALSE, NULL);
}

/**
 * lightdm_shutdown:
 * @error: return location for a #GError, or %NULL
 *
 * Triggers a system shutdown.
 * 
 * Return value: #TRUE if shutdown initiated.
 **/
gboolean
lightdm_shutdown (GError **error)
{
    return ck_call_function ("Stop", TRUE, error);
}
