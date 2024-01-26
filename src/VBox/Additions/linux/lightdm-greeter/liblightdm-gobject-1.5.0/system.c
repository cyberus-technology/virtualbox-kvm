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

#include <sys/utsname.h>

#include "lightdm/system.h"

static gchar *hostname = NULL;

/**
 * lightdm_get_hostname:
 *
 * Return value: The name of the host we are running on.
 **/
const gchar *
lightdm_get_hostname (void)
{
    if (!hostname)
    {
        struct utsname info;
        uname (&info);
        hostname = g_strdup (info.nodename);
    }

    return hostname;
}
