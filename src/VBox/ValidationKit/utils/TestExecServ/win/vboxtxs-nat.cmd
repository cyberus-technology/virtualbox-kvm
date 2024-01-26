@REM REM @file
@REM VirtualBox Test Execution Service Init Script for NATted VMs.
@REM

@REM
REM
REM Copyright (C) 2006-2023 Oracle and/or its affiliates.
REM
REM This file is part of VirtualBox base platform packages, as
REM available from https://www.virtualbox.org.
REM
REM This program is free software; you can redistribute it and/or
REM modify it under the terms of the GNU General Public License
REM as published by the Free Software Foundation, in version 3 of the
REM License.
REM
REM This program is distributed in the hope that it will be useful, but
REM WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
REM General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with this program; if not, see <https://www.gnu.org/licenses>.
REM
REM The contents of this file may alternatively be used under the terms
REM of the Common Development and Distribution License Version 1.0
REM (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
REM in the VirtualBox distribution, in which case the provisions of the
REM CDDL are applicable instead of those of the GPL.
REM
REM You may elect to license modified versions of this file under the
REM terms and conditions of either the GPL or the CDDL or both.
REM
REM SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
REM

%SystemDrive%\Apps\TestExecService.exe --foreground --display-output ^
--cdrom D:\ --scratch C:\Temp\vboxtest --auto-upgrade ^
--tcp-connect 10.0.2.2
pause

