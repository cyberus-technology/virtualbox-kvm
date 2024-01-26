/** @file
 *
 * VirtualBox Timesync using temporary Backdoor
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

#include <unistd.h>
#include <asm/io.h>
#include <sys/time.h>
#include <time.h>

void usage()
{
    printf("TimesyncBackdoor [-interval <seconds>]"
           "                 [-daemonize]"
           "\n");
}

int main(int argc, char *argv[])
{
    int secInterval = 10;
    int fDaemonize = 0;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-interval") == 0)
        {
            if (argc <= i)
            {
                usage();
                return 1;
            }
            secInterval = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-daemonize") == 0)
        {
            fDaemonize = 1;
        }
        else
        {
            usage();
            return 1;
        }
    }

    /* get port IO permission */
    if (iopl(3))
    {
        printf("Error: could not set IOPL to 3!\n");
        return 1;
    }

    printf("VirtualBox timesync tool. Sync interval: %d seconds.\n", secInterval);

    if (fDaemonize)
        daemon(1, 0);

    do
    {
        unsigned long long time;
        /* get the high 32bit, this _must_ be done first */
        outl(0, 0x505);
        time = (unsigned long long)inl(0x505) << 32;
        /* get the low 32bit */
        outl(1, 0x505);
        time |= inl(0x505);

        /* set the system time */
        struct timeval tv;
        tv.tv_sec  = time / (unsigned long long)1000;
        tv.tv_usec = (time % (unsigned long long)1000) * 1000;
        settimeofday(&tv, NULL);

    /* wait for the next run */
        sleep(secInterval);

    } while (1);

    return 0;
}
