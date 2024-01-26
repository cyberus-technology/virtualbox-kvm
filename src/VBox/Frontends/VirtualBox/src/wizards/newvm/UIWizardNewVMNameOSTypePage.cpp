/* $Id: UIWizardNewVMNameOSTypePage.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasicNameOSStype class implementation.
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

/* Qt includes: */
#include <QCheckBox>
#include <QDir>
#include <QRegularExpression>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIRichTextLabel.h"
#include "UICommon.h"
#include "UINameAndSystemEditor.h"
#include "UINotificationCenter.h"
#include "UIWizardNewVMNameOSTypePage.h"
#include "UIWizardNewVM.h"

/* COM includes: */
#include "CHost.h"
#include "CUnattended.h"

/* Defines some patterns to guess the right OS type. Should be in sync with
 * VirtualBox-settings-common.xsd in Main. The list is sorted by priority. The
 * first matching string found, will be used. */
struct osTypePattern
{
    QRegularExpression pattern;
    const char *pcstId;
};

static const osTypePattern gs_OSTypePattern[] =
{
    /* DOS: */
    { QRegularExpression("DOS", QRegularExpression::CaseInsensitiveOption), "DOS" },

    /* Windows: */
    { QRegularExpression(  "Wi.*98",                         QRegularExpression::CaseInsensitiveOption), "Windows98" },
    { QRegularExpression(  "Wi.*95",                         QRegularExpression::CaseInsensitiveOption), "Windows95" },
    { QRegularExpression(  "Wi.*Me",                         QRegularExpression::CaseInsensitiveOption), "WindowsMe" },
    { QRegularExpression( "(Wi.*NT)|(NT[-._v]*4)",           QRegularExpression::CaseInsensitiveOption), "WindowsNT4" },
    { QRegularExpression( "NT[-._v]*3[.,]*[51x]",            QRegularExpression::CaseInsensitiveOption), "WindowsNT3x" },
    { QRegularExpression("(Wi.*XP.*64)|(XP.*64)",            QRegularExpression::CaseInsensitiveOption), "WindowsXP_64" },
    { QRegularExpression("(XP)",                             QRegularExpression::CaseInsensitiveOption), "WindowsXP" },
    { QRegularExpression("((Wi.*2003)|(W2K3)|(Win2K3)).*64", QRegularExpression::CaseInsensitiveOption), "Windows2003_64" },
    { QRegularExpression("((Wi.*2003)|(W2K3)|(Win2K3)).*32", QRegularExpression::CaseInsensitiveOption), "Windows2003" },
    { QRegularExpression("((Wi.*Vis)|(Vista)).*64",          QRegularExpression::CaseInsensitiveOption), "WindowsVista_64" },
    { QRegularExpression("((Wi.*Vis)|(Vista)).*32",          QRegularExpression::CaseInsensitiveOption), "WindowsVista" },
    { QRegularExpression( "(Wi.*2016)|(W2K16)|(Win2K16)",    QRegularExpression::CaseInsensitiveOption), "Windows2016_64" },
    { QRegularExpression( "(Wi.*2012)|(W2K12)|(Win2K12)",    QRegularExpression::CaseInsensitiveOption), "Windows2012_64" },
    { QRegularExpression("((Wi.*2008)|(W2K8)|(Win2k8)).*64", QRegularExpression::CaseInsensitiveOption), "Windows2008_64" },
    { QRegularExpression("((Wi.*2008)|(W2K8)|(Win2K8)).*32", QRegularExpression::CaseInsensitiveOption), "Windows2008" },
    { QRegularExpression( "(Wi.*2000)|(W2K)|(Win2K)",        QRegularExpression::CaseInsensitiveOption), "Windows2000" },
    { QRegularExpression( "(Wi.*7.*64)|(W7.*64)",            QRegularExpression::CaseInsensitiveOption), "Windows7_64" },
    { QRegularExpression( "(Wi.*7.*32)|(W7.*32)",            QRegularExpression::CaseInsensitiveOption), "Windows7" },
    { QRegularExpression( "(Wi.*8.*1.*64)|(W8.*64)",         QRegularExpression::CaseInsensitiveOption), "Windows81_64" },
    { QRegularExpression( "(Wi.*8.*1.*32)|(W8.*32)",         QRegularExpression::CaseInsensitiveOption), "Windows81" },
    { QRegularExpression( "(Wi.*8.*64)|(W8.*64)",            QRegularExpression::CaseInsensitiveOption), "Windows8_64" },
    { QRegularExpression( "(Wi.*8.*32)|(W8.*32)",            QRegularExpression::CaseInsensitiveOption), "Windows8" },
    { QRegularExpression( "(Wi.*10.*64)|(W10.*64)",          QRegularExpression::CaseInsensitiveOption), "Windows10_64" },
    { QRegularExpression( "(Wi.*10.*32)|(W10.*32)",          QRegularExpression::CaseInsensitiveOption), "Windows10" },
    { QRegularExpression( "(Wi.*11)|(W11)",                  QRegularExpression::CaseInsensitiveOption), "Windows11_64" },
    { QRegularExpression(  "Wi.*3.*1",                       QRegularExpression::CaseInsensitiveOption), "Windows31" },
    /* Set Windows 10 as default for "Windows". */
    { QRegularExpression(  "Wi.*64",                         QRegularExpression::CaseInsensitiveOption), "Windows10_64" },
    { QRegularExpression(  "Wi.*32",                         QRegularExpression::CaseInsensitiveOption), "Windows10" },
    /* ReactOS wants to be considered as Windows 2003 */
    { QRegularExpression(  "Reac.*",                         QRegularExpression::CaseInsensitiveOption), "Windows2003" },

    /* Solaris: */
    { QRegularExpression("((Op.*Sol)|(os20[01][0-9])|(India)|(Illum)|(Neva)).*64",   QRegularExpression::CaseInsensitiveOption), "OpenSolaris_64" },
    { QRegularExpression("((Op.*Sol)|(os20[01][0-9])|(India)|(Illum)|(Neva)).*32",   QRegularExpression::CaseInsensitiveOption), "OpenSolaris" },
    { QRegularExpression("(Sol.*10.*(10/09)|(9/10)|(8/11)|(1/13)).*64",              QRegularExpression::CaseInsensitiveOption), "Solaris10U8_or_later_64" },
    { QRegularExpression("(Sol.*10.*(10/09)|(9/10)|(8/11)|(1/13)).*32",              QRegularExpression::CaseInsensitiveOption), "Solaris10U8_or_later" },
    { QRegularExpression("(Sol.*10.*(U[89])|(U1[01])).*64",                          QRegularExpression::CaseInsensitiveOption), "Solaris10U8_or_later_64" },
    { QRegularExpression("(Sol.*10.*(U[89])|(U1[01])).*32",                          QRegularExpression::CaseInsensitiveOption), "Solaris10U8_or_later" },
    { QRegularExpression("(Sol.*10.*(1/06)|(6/06)|(11/06)|(8/07)|(5/08)|(10/08)|(5/09)).*64",  QRegularExpression::CaseInsensitiveOption), "Solaris_64" }, // Solaris 10U7 (5/09) or earlier
    { QRegularExpression("(Sol.*10.*(1/06)|(6/06)|(11/06)|(8/07)|(5/08)|(10/08)|(5/09)).*32",  QRegularExpression::CaseInsensitiveOption), "Solaris" }, // Solaris 10U7 (5/09) or earlier
    { QRegularExpression("((Sol.*10.*U[1-7])|(Sol.*10)).*64",                        QRegularExpression::CaseInsensitiveOption), "Solaris_64" }, // Solaris 10U7 (5/09) or earlier
    { QRegularExpression("((Sol.*10.*U[1-7])|(Sol.*10)).*32",                        QRegularExpression::CaseInsensitiveOption), "Solaris" }, // Solaris 10U7 (5/09) or earlier
    { QRegularExpression("((Sol.*11)|(Sol.*)).*64",                                  QRegularExpression::CaseInsensitiveOption), "Solaris11_64" },

    /* OS/2: */
    { QRegularExpression("OS[/|!-]{,1}2.*W.*4.?5", QRegularExpression::CaseInsensitiveOption), "OS2Warp45" },
    { QRegularExpression("OS[/|!-]{,1}2.*W.*4",    QRegularExpression::CaseInsensitiveOption), "OS2Warp4" },
    { QRegularExpression("OS[/|!-]{,1}2.*W",       QRegularExpression::CaseInsensitiveOption), "OS2Warp3" },
    { QRegularExpression("OS[/|!-]{,1}2.*e",       QRegularExpression::CaseInsensitiveOption), "OS2eCS" },
    { QRegularExpression("OS[/|!-]{,1}2.*Ar.*",    QRegularExpression::CaseInsensitiveOption), "OS2ArcaOS" },
    { QRegularExpression("OS[/|!-]{,1}2",          QRegularExpression::CaseInsensitiveOption), "OS2" },
    { QRegularExpression("(eComS.*|eCS.*)",        QRegularExpression::CaseInsensitiveOption), "OS2eCS" },
    { QRegularExpression("Arca.*",                 QRegularExpression::CaseInsensitiveOption), "OS2ArcaOS" },

    /* Other: Must come before Ubuntu/Maverick and before Linux??? */
    { QRegularExpression("QN", QRegularExpression::CaseInsensitiveOption), "QNX" },

    /* Mac OS X: Must come before Ubuntu/Maverick and before Linux: */
    { QRegularExpression("((mac.*10[.,]{0,1}4)|(os.*x.*10[.,]{0,1}4)|(mac.*ti)|(os.*x.*ti)|(Tig)).64",     QRegularExpression::CaseInsensitiveOption), "MacOS_64" },
    { QRegularExpression("((mac.*10[.,]{0,1}4)|(os.*x.*10[.,]{0,1}4)|(mac.*ti)|(os.*x.*ti)|(Tig)).32",     QRegularExpression::CaseInsensitiveOption), "MacOS" },
    { QRegularExpression("((mac.*10[.,]{0,1}5)|(os.*x.*10[.,]{0,1}5)|(mac.*leo)|(os.*x.*leo)|(Leop)).*64", QRegularExpression::CaseInsensitiveOption), "MacOS_64" },
    { QRegularExpression("((mac.*10[.,]{0,1}5)|(os.*x.*10[.,]{0,1}5)|(mac.*leo)|(os.*x.*leo)|(Leop)).*32", QRegularExpression::CaseInsensitiveOption), "MacOS" },
    { QRegularExpression("((mac.*10[.,]{0,1}6)|(os.*x.*10[.,]{0,1}6)|(mac.*SL)|(os.*x.*SL)|(Snow L)).*64", QRegularExpression::CaseInsensitiveOption), "MacOS106_64" },
    { QRegularExpression("((mac.*10[.,]{0,1}6)|(os.*x.*10[.,]{0,1}6)|(mac.*SL)|(os.*x.*SL)|(Snow L)).*32", QRegularExpression::CaseInsensitiveOption), "MacOS106" },
    { QRegularExpression( "(mac.*10[.,]{0,1}7)|(os.*x.*10[.,]{0,1}7)|(mac.*ML)|(os.*x.*ML)|(Mount)",       QRegularExpression::CaseInsensitiveOption), "MacOS107_64" },
    { QRegularExpression( "(mac.*10[.,]{0,1}8)|(os.*x.*10[.,]{0,1}8)|(Lion)",                              QRegularExpression::CaseInsensitiveOption), "MacOS108_64" },
    { QRegularExpression( "(mac.*10[.,]{0,1}9)|(os.*x.*10[.,]{0,1}9)|(mac.*mav)|(os.*x.*mav)|(Mavericks)", QRegularExpression::CaseInsensitiveOption), "MacOS109_64" },
    { QRegularExpression( "(mac.*yos)|(os.*x.*yos)|(Yosemite)",                                            QRegularExpression::CaseInsensitiveOption), "MacOS1010_64" },
    { QRegularExpression( "(mac.*cap)|(os.*x.*capit)|(Capitan)",                                           QRegularExpression::CaseInsensitiveOption), "MacOS1011_64" },
    { QRegularExpression( "(mac.*hig)|(os.*x.*high.*sierr)|(High Sierra)",                                 QRegularExpression::CaseInsensitiveOption), "MacOS1013_64" },
    { QRegularExpression( "(mac.*sie)|(os.*x.*sierr)|(Sierra)",                                            QRegularExpression::CaseInsensitiveOption), "MacOS1012_64" },
    { QRegularExpression("((Mac)|(Tig)|(Leop)|(Yose)|(os[ ]*x)).*64",                                      QRegularExpression::CaseInsensitiveOption), "MacOS_64" },
    { QRegularExpression("((Mac)|(Tig)|(Leop)|(Yose)|(os[ ]*x)).*32",                                      QRegularExpression::CaseInsensitiveOption), "MacOS" },

    /* Code names for Linux distributions: */
    { QRegularExpression("((bianca)|(cassandra)|(celena)|(daryna)|(elyssa)|(felicia)|(gloria)|(helena)|(isadora)|(julia)|(katya)|(lisa)|(maya)|(nadia)|(olivia)|(petra)|(qiana)|(rebecca)|(rafaela)|(rosa)).*64", QRegularExpression::CaseInsensitiveOption), "Ubuntu_64" },
    { QRegularExpression("((bianca)|(cassandra)|(celena)|(daryna)|(elyssa)|(felicia)|(gloria)|(helena)|(isadora)|(julia)|(katya)|(lisa)|(maya)|(nadia)|(olivia)|(petra)|(qiana)|(rebecca)|(rafaela)|(rosa)).*32", QRegularExpression::CaseInsensitiveOption), "Ubuntu" },
    { QRegularExpression("((edgy)|(feisty)|(gutsy)|(hardy)|(intrepid)|(jaunty)|(karmic)).*64",  QRegularExpression::CaseInsensitiveOption), "Ubuntu_64" },
    { QRegularExpression("((edgy)|(feisty)|(gutsy)|(hardy)|(intrepid)|(jaunty)|(karmic)).*32",  QRegularExpression::CaseInsensitiveOption), "Ubuntu" },
    { QRegularExpression("((eft)|(fawn)|(gibbon)|(heron)|(ibex)|(jackalope)|(koala)).*64",      QRegularExpression::CaseInsensitiveOption), "Ubuntu_64" },
    { QRegularExpression("((eft)|(fawn)|(gibbon)|(heron)|(ibex)|(jackalope)|(koala)).*32",      QRegularExpression::CaseInsensitiveOption), "Ubuntu" },
    { QRegularExpression("((lucid)|(lynx)).*64",                                                QRegularExpression::CaseInsensitiveOption), "Ubuntu10_LTS_64" },
    { QRegularExpression("((lucid)|(lynx)).*32",                                                QRegularExpression::CaseInsensitiveOption), "Ubuntu10_LTS" },
    { QRegularExpression("((maverick)|(meerkat)).*64",                                          QRegularExpression::CaseInsensitiveOption), "Ubuntu10_64" },
    { QRegularExpression("((maverick)|(meerkat)).*32",                                          QRegularExpression::CaseInsensitiveOption), "Ubuntu10" },
    { QRegularExpression("((natty)|(narwhal)|(oneiric)|(ocelot)).*64",                          QRegularExpression::CaseInsensitiveOption), "Ubuntu11_64" },
    { QRegularExpression("((natty)|(narwhal)|(oneiric)|(ocelot)).*32",                          QRegularExpression::CaseInsensitiveOption), "Ubuntu11" },
    { QRegularExpression("((precise)|(pangolin)).*64",                                          QRegularExpression::CaseInsensitiveOption), "Ubuntu12_LTS_64" },
    { QRegularExpression("((precise)|(pangolin)).*32",                                          QRegularExpression::CaseInsensitiveOption), "Ubuntu12_LTS" },
    { QRegularExpression("((quantal)|(quetzal)).*64",                                           QRegularExpression::CaseInsensitiveOption), "Ubuntu12_64" },
    { QRegularExpression("((quantal)|(quetzal)).*32",                                           QRegularExpression::CaseInsensitiveOption), "Ubuntu12" },
    { QRegularExpression("((raring)|(ringtail)|(saucy)|(salamander)).*64",                      QRegularExpression::CaseInsensitiveOption), "Ubuntu13_64" },
    { QRegularExpression("((raring)|(ringtail)|(saucy)|(salamander)).*32",                      QRegularExpression::CaseInsensitiveOption), "Ubuntu13" },
    { QRegularExpression("((trusty)|(tahr)).*64",                                               QRegularExpression::CaseInsensitiveOption), "Ubuntu14_LTS_64" },
    { QRegularExpression("((trusty)|(tahr)).*32",                                               QRegularExpression::CaseInsensitiveOption), "Ubuntu14_LTS" },
    { QRegularExpression("((utopic)|(unicorn)).*64",                                            QRegularExpression::CaseInsensitiveOption), "Ubuntu14_64" },
    { QRegularExpression("((utopic)|(unicorn)).*32",                                            QRegularExpression::CaseInsensitiveOption), "Ubuntu14" },
    { QRegularExpression("((vivid)|(vervet)|(wily)|(werewolf)).*64",                            QRegularExpression::CaseInsensitiveOption), "Ubuntu15_64" },
    { QRegularExpression("((vivid)|(vervet)|(wily)|(werewolf)).*32",                            QRegularExpression::CaseInsensitiveOption), "Ubuntu15" },
    { QRegularExpression("((xenial)|(xerus)).*64",                                              QRegularExpression::CaseInsensitiveOption), "Ubuntu16_LTS_64" },
    { QRegularExpression("((xenial)|(xerus)).*32",                                              QRegularExpression::CaseInsensitiveOption), "Ubuntu16_LTS" },
    { QRegularExpression("((yakkety)|(yak)).*64",                                               QRegularExpression::CaseInsensitiveOption), "Ubuntu16_64" },
    { QRegularExpression("((yakkety)|(yak)).*32",                                               QRegularExpression::CaseInsensitiveOption), "Ubuntu16" },
    { QRegularExpression("((zesty)|(zapus)|(artful)|(aardvark)).*64",                           QRegularExpression::CaseInsensitiveOption), "Ubuntu17_64" },
    { QRegularExpression("((zesty)|(zapus)|(artful)|(aardvark)).*32",                           QRegularExpression::CaseInsensitiveOption), "Ubuntu17" },
    { QRegularExpression("((bionic)|(beaver)).*64",                                             QRegularExpression::CaseInsensitiveOption), "Ubuntu18_LTS_64" },
    { QRegularExpression("((bionic)|(beaver)).*32",                                             QRegularExpression::CaseInsensitiveOption), "Ubuntu18_LTS" },
    { QRegularExpression("((cosmic)|(cuttlefish)).*64",                                         QRegularExpression::CaseInsensitiveOption), "Ubuntu18_64" },
    { QRegularExpression("((cosmic)|(cuttlefish)).*32",                                         QRegularExpression::CaseInsensitiveOption), "Ubuntu18" },
    { QRegularExpression("((disco)|(dingo)|(eoan)|(ermine)).*64",                               QRegularExpression::CaseInsensitiveOption), "Ubuntu19_64" },
    { QRegularExpression("((disco)|(dingo)|(eoan)|(ermine)).*32",                               QRegularExpression::CaseInsensitiveOption), "Ubuntu19" },
    { QRegularExpression("((focal)|(fossa)).*64",                                               QRegularExpression::CaseInsensitiveOption), "Ubuntu20_LTS_64" },
    { QRegularExpression("((groovy)|(gorilla)).*64",                                            QRegularExpression::CaseInsensitiveOption), "Ubuntu20_64" },
    { QRegularExpression("((hirsute)|(hippo)|(impish)|(indri)).*64",                            QRegularExpression::CaseInsensitiveOption), "Ubuntu21_64" },
    { QRegularExpression("((jammy)|(jellyfish)).*64",                                           QRegularExpression::CaseInsensitiveOption), "Ubuntu22_LTS_64" },
    { QRegularExpression("((kinetic)|(kudu)).*64",                                              QRegularExpression::CaseInsensitiveOption), "Ubuntu22_64" },
    { QRegularExpression("((lunar)|(lobster)).*64",                                             QRegularExpression::CaseInsensitiveOption), "Ubuntu23_64" },
    { QRegularExpression("sarge.*32",                         QRegularExpression::CaseInsensitiveOption), "Debian31" },
    { QRegularExpression("^etch.*64",                         QRegularExpression::CaseInsensitiveOption), "Debian4_64" },
    { QRegularExpression("^etch.*32",                         QRegularExpression::CaseInsensitiveOption), "Debian4" },
    { QRegularExpression("lenny.*64",                         QRegularExpression::CaseInsensitiveOption), "Debian5_64" },
    { QRegularExpression("lenny.*32",                         QRegularExpression::CaseInsensitiveOption), "Debian5" },
    { QRegularExpression("squeeze.*64",                       QRegularExpression::CaseInsensitiveOption), "Debian6_64" },
    { QRegularExpression("squeeze.*32",                       QRegularExpression::CaseInsensitiveOption), "Debian6" },
    { QRegularExpression("wheezy.*64",                        QRegularExpression::CaseInsensitiveOption), "Debian7_64" },
    { QRegularExpression("wheezy.*32",                        QRegularExpression::CaseInsensitiveOption), "Debian7" },
    { QRegularExpression("jessie.*64",                        QRegularExpression::CaseInsensitiveOption), "Debian8_64" },
    { QRegularExpression("jessie.*32",                        QRegularExpression::CaseInsensitiveOption), "Debian8" },
    { QRegularExpression("stretch.*64",                       QRegularExpression::CaseInsensitiveOption), "Debian9_64" },
    { QRegularExpression("stretch.*32",                       QRegularExpression::CaseInsensitiveOption), "Debian9" },
    { QRegularExpression("buster.*64",                        QRegularExpression::CaseInsensitiveOption), "Debian10_64" },
    { QRegularExpression("buster.*32",                        QRegularExpression::CaseInsensitiveOption), "Debian10" },
    { QRegularExpression("bullseye.*64",                      QRegularExpression::CaseInsensitiveOption), "Debian11_64" },
    { QRegularExpression("bullseye.*32",                      QRegularExpression::CaseInsensitiveOption), "Debian11" },
    { QRegularExpression("bookworm.*64",                      QRegularExpression::CaseInsensitiveOption), "Debian12_64" },
    { QRegularExpression("bookworm.*32",                      QRegularExpression::CaseInsensitiveOption), "Debian12" },
    { QRegularExpression("((trixie)|(sid)).*64",              QRegularExpression::CaseInsensitiveOption), "Debian_64" },
    { QRegularExpression("((trixie)|(sid)).*32",              QRegularExpression::CaseInsensitiveOption), "Debian" },
    { QRegularExpression("((moonshine)|(werewolf)|(sulphur)|(cambridge)|(leonidas)|(constantine)|(goddard)|(laughlin)|(lovelock)|(verne)|(beefy)|(spherical)|(schrodinger)|(heisenberg)).*64", QRegularExpression::CaseInsensitiveOption), "Fedora_64" },
    { QRegularExpression("((moonshine)|(werewolf)|(sulphur)|(cambridge)|(leonidas)|(constantine)|(goddard)|(laughlin)|(lovelock)|(verne)|(beefy)|(spherical)|(schrodinger)|(heisenberg)).*32", QRegularExpression::CaseInsensitiveOption), "Fedora" },
    { QRegularExpression("((basilisk)|(emerald)|(teal)|(celadon)|(asparagus)|(mantis)|(dartmouth)|(bottle)|(harlequin)).*64", QRegularExpression::CaseInsensitiveOption), "OpenSUSE_64" },
    { QRegularExpression("((basilisk)|(emerald)|(teal)|(celadon)|(asparagus)|(mantis)|(dartmouth)|(bottle)|(harlequin)).*32", QRegularExpression::CaseInsensitiveOption), "OpenSUSE" },

    /* Regular names of Linux distributions: */
    { QRegularExpression("Arc.*64",                           QRegularExpression::CaseInsensitiveOption), "ArchLinux_64" },
    { QRegularExpression("Arc.*32",                           QRegularExpression::CaseInsensitiveOption), "ArchLinux" },
    { QRegularExpression("Deb.*64",                           QRegularExpression::CaseInsensitiveOption), "Debian_64" },
    { QRegularExpression("Deb.*32",                           QRegularExpression::CaseInsensitiveOption), "Debian" },
    { QRegularExpression("SU.*Leap.*64",                      QRegularExpression::CaseInsensitiveOption), "OpenSUSE_Leap_64" },
    { QRegularExpression("SU.*Tumble.*64",                    QRegularExpression::CaseInsensitiveOption), "OpenSUSE_Tumbleweed_64" },
    { QRegularExpression("SU.*Tumble.*32",                    QRegularExpression::CaseInsensitiveOption), "OpenSUSE_Tumbleweed" },
    { QRegularExpression("((SU)|(Nov)|(SLE)).*64",            QRegularExpression::CaseInsensitiveOption), "OpenSUSE_64" },
    { QRegularExpression("((SU)|(Nov)|(SLE)).*32",            QRegularExpression::CaseInsensitiveOption), "OpenSUSE" },
    { QRegularExpression("Fe.*64",                            QRegularExpression::CaseInsensitiveOption), "Fedora_64" },
    { QRegularExpression("Fe.*32",                            QRegularExpression::CaseInsensitiveOption), "Fedora" },
    { QRegularExpression("((Gen)|(Sab)).*64",                 QRegularExpression::CaseInsensitiveOption), "Gentoo_64" },
    { QRegularExpression("((Gen)|(Sab)).*32",                 QRegularExpression::CaseInsensitiveOption), "Gentoo" },
    { QRegularExpression("^Man.*64",                          QRegularExpression::CaseInsensitiveOption), "Mandriva_64" },
    { QRegularExpression("^Man.*32",                          QRegularExpression::CaseInsensitiveOption), "Mandriva" },
    { QRegularExpression("Op.*Man.*Lx.*64",                   QRegularExpression::CaseInsensitiveOption), "OpenMandriva_Lx_64" },
    { QRegularExpression("Op.*Man.*Lx.*32",                   QRegularExpression::CaseInsensitiveOption), "OpenMandriva_Lx" },
    { QRegularExpression("PCL.*OS.*64",                       QRegularExpression::CaseInsensitiveOption), "PCLinuxOS_64" },
    { QRegularExpression("PCL.*OS.*32",                       QRegularExpression::CaseInsensitiveOption), "PCLinuxOS" },
    { QRegularExpression("Mageia.*64",                        QRegularExpression::CaseInsensitiveOption), "Mageia_64" },
    { QRegularExpression("Mageia.*32",                        QRegularExpression::CaseInsensitiveOption), "Mageia" },
    { QRegularExpression("((Red)|(rhel)|(cen)).*64",          QRegularExpression::CaseInsensitiveOption), "RedHat_64" },
    { QRegularExpression("((Red)|(rhel)|(cen)).*32",          QRegularExpression::CaseInsensitiveOption), "RedHat" },
    { QRegularExpression("Tur.*64",                           QRegularExpression::CaseInsensitiveOption), "Turbolinux_64" },
    { QRegularExpression("Tur.*32",                           QRegularExpression::CaseInsensitiveOption), "Turbolinux" },
    { QRegularExpression("Lub.*64",                           QRegularExpression::CaseInsensitiveOption), "Lubuntu_64" },
    { QRegularExpression("Lub.*32",                           QRegularExpression::CaseInsensitiveOption), "Lubuntu" },
    { QRegularExpression("Xub.*64",                           QRegularExpression::CaseInsensitiveOption), "Xubuntu_64" },
    { QRegularExpression("Xub.*32",                           QRegularExpression::CaseInsensitiveOption), "Xubuntu" },
    { QRegularExpression("((Ub)|(Mint)).*64",                 QRegularExpression::CaseInsensitiveOption), "Ubuntu_64" },
    { QRegularExpression("((Ub)|(Mint)).*32",                 QRegularExpression::CaseInsensitiveOption), "Ubuntu" },
    { QRegularExpression("Xa.*64",                            QRegularExpression::CaseInsensitiveOption), "Xandros_64" },
    { QRegularExpression("Xa.*32",                            QRegularExpression::CaseInsensitiveOption), "Xandros" },
    { QRegularExpression("((Or)|(oel)|(^ol)).*64",            QRegularExpression::CaseInsensitiveOption), "Oracle_64" },
    { QRegularExpression("((Or)|(oel)|(^ol)).*32",            QRegularExpression::CaseInsensitiveOption), "Oracle" },
    { QRegularExpression("Knoppix",                           QRegularExpression::CaseInsensitiveOption), "Linux26" },
    { QRegularExpression("Dsl",                               QRegularExpression::CaseInsensitiveOption), "Linux24" },
    { QRegularExpression("((Lin)|(lnx)).*2.?2",               QRegularExpression::CaseInsensitiveOption), "Linux22" },
    { QRegularExpression("((Lin)|(lnx)).*2.?4.*64",           QRegularExpression::CaseInsensitiveOption), "Linux24_64" },
    { QRegularExpression("((Lin)|(lnx)).*2.?4.*32",           QRegularExpression::CaseInsensitiveOption), "Linux24" },
    { QRegularExpression("((((Lin)|(lnx)).*2.?6)|(LFS)).*64", QRegularExpression::CaseInsensitiveOption), "Linux26_64" },
    { QRegularExpression("((((Lin)|(lnx)).*2.?6)|(LFS)).*32", QRegularExpression::CaseInsensitiveOption), "Linux26" },
    { QRegularExpression("((Lin)|(lnx)).*64",                 QRegularExpression::CaseInsensitiveOption), "Linux26_64" },
    { QRegularExpression("((Lin)|(lnx)).*32",                 QRegularExpression::CaseInsensitiveOption), "Linux26" },

    /* Other: */
    { QRegularExpression("L4",                   QRegularExpression::CaseInsensitiveOption), "L4" },
    { QRegularExpression("((Fr.*B)|(fbsd)).*64", QRegularExpression::CaseInsensitiveOption), "FreeBSD_64" },
    { QRegularExpression("((Fr.*B)|(fbsd)).*32", QRegularExpression::CaseInsensitiveOption), "FreeBSD" },
    { QRegularExpression("Op.*B.*64",            QRegularExpression::CaseInsensitiveOption), "OpenBSD_64" },
    { QRegularExpression("Op.*B.*32",            QRegularExpression::CaseInsensitiveOption), "OpenBSD" },
    { QRegularExpression("Ne.*B.*64",            QRegularExpression::CaseInsensitiveOption), "NetBSD_64" },
    { QRegularExpression("Ne.*B.*32",            QRegularExpression::CaseInsensitiveOption), "NetBSD" },
    { QRegularExpression("Net",                  QRegularExpression::CaseInsensitiveOption), "Netware" },
    { QRegularExpression("Rocki",                QRegularExpression::CaseInsensitiveOption), "JRockitVE" },
    { QRegularExpression("bs[23]{0,1}-",         QRegularExpression::CaseInsensitiveOption), "VBoxBS_64" }, /* bootsector tests */
    { QRegularExpression("Ot",                   QRegularExpression::CaseInsensitiveOption), "Other" },
};

static const QRegularExpression gs_Prefer32BitNamePatterns = QRegularExpression("(XP)", QRegularExpression::CaseInsensitiveOption);


bool UIWizardNewVMNameOSTypeCommon::guessOSTypeFromName(UINameAndSystemEditor *pNameAndSystemEditor, QString strNewName)
{
    AssertReturn(pNameAndSystemEditor, false);

    /* Append default architecture bit-count (64/32) if not already in the name, unless
       it's XP or similar which is predominantly 32-bit: */
    if (!strNewName.contains("32") && !strNewName.contains("64") && !strNewName.contains(gs_Prefer32BitNamePatterns))
        strNewName += ARCH_BITS == 64 ? "64" : "32";

    /* Search for a matching OS type based on the string the user typed already. */
    for (size_t i = 0; i < RT_ELEMENTS(gs_OSTypePattern); ++i)
    {
        if (strNewName.contains(gs_OSTypePattern[i].pattern))
        {
            pNameAndSystemEditor->setType(uiCommon().vmGuestOSType(gs_OSTypePattern[i].pcstId));
            return true;
        }
    }
    return false;
}

bool UIWizardNewVMNameOSTypeCommon::guessOSTypeDetectedOSTypeString(UINameAndSystemEditor *pNameAndSystemEditor, QString strDetectedOSType)
{
    AssertReturn(pNameAndSystemEditor, false);
    if (!strDetectedOSType.isEmpty())
    {
        CGuestOSType const osType = uiCommon().vmGuestOSType(strDetectedOSType);
        if (!osType.isNull())
        {
            pNameAndSystemEditor->setType(osType);
            return true;
        }
        /* The detectedOSType shall be a valid OS type ID. So, unless the UI is
           out of sync with the types in main this shouldn't ever happen. */
        AssertFailed();
    }
    pNameAndSystemEditor->setType(uiCommon().vmGuestOSType("Other"));
    /* Return false to allow OS type guessing from name. See caller code: */
    return false;
}

void UIWizardNewVMNameOSTypeCommon::composeMachineFilePath(UINameAndSystemEditor *pNameAndSystemEditor,
                                                           UIWizardNewVM *pWizard)
{
    if (!pNameAndSystemEditor || !pWizard)
        return;
    if (pNameAndSystemEditor->name().isEmpty() || pNameAndSystemEditor->path().isEmpty())
        return;
    /* Get VBox: */
    CVirtualBox vbox = uiCommon().virtualBox();

    /* Compose machine filename: */
    pWizard->setMachineFilePath(vbox.ComposeMachineFilename(pNameAndSystemEditor->name(),
                                                            pWizard->machineGroup(),
                                                            QString(),
                                                            pNameAndSystemEditor->path()));
    /* Compose machine folder/basename: */
    const QFileInfo fileInfo(pWizard->machineFilePath());
    pWizard->setMachineFolder(fileInfo.absolutePath());
    pWizard->setMachineBaseName(pNameAndSystemEditor->name());
}

bool UIWizardNewVMNameOSTypeCommon::createMachineFolder(UINameAndSystemEditor *pNameAndSystemEditor,
                                                        UIWizardNewVM *pWizard)
{
    if (!pNameAndSystemEditor || !pWizard)
        return false;
    const QString &strMachineFolder = pWizard->machineFolder();
    const QString &strCreatedFolder = pWizard->createdMachineFolder();

    /* Cleanup previosly created folder if any: */
    if (!cleanupMachineFolder(pWizard))
    {
        UINotificationMessage::cannotRemoveMachineFolder(strCreatedFolder, pWizard->notificationCenter());
        return false;
    }

    /* Check if the folder already exists and check if it has been created by this wizard */
    if (QDir(strMachineFolder).exists())
    {
        /* Looks like we have already created this folder for this run of the wizard. Just return */
        if (strCreatedFolder == strMachineFolder)
            return true;
        /* The folder is there but not because of this wizard. Avoid overwriting a existing machine's folder */
        else
        {
            UINotificationMessage::cannotOverwriteMachineFolder(strMachineFolder, pWizard->notificationCenter());
            return false;
        }
    }

    /* Try to create new folder (and it's predecessors): */
    bool fMachineFolderCreated = QDir().mkpath(strMachineFolder);
    if (!fMachineFolderCreated)
    {
        UINotificationMessage::cannotCreateMachineFolder(strMachineFolder, pWizard->notificationCenter());
        return false;
    }
    pWizard->setCreatedMachineFolder(strMachineFolder);
    return true;
}

bool UIWizardNewVMNameOSTypeCommon::cleanupMachineFolder(UIWizardNewVM *pWizard, bool fWizardCancel /* = false */)
{
    if (!pWizard)
        return false;
    const QString &strMachineFolder = pWizard->machineFolder();
    const QString &strCreatedFolder = pWizard->createdMachineFolder();
    /* Make sure folder was previosly created: */
    if (strCreatedFolder.isEmpty())
        return true;
    /* Clean this folder if the machine folder has been changed by the user or we are cancelling the wizard: */
    if (strCreatedFolder != strMachineFolder || fWizardCancel)
    {
        /* Try to cleanup folder (and it's predecessors): */
        bool fMachineFolderRemoved = QDir(strCreatedFolder).removeRecursively();
        /* Reset machine folder value: */
        if (fMachineFolderRemoved)
            pWizard->setCreatedMachineFolder(QString());
        /* Return cleanup result: */
        return fMachineFolderRemoved;
    }
    return true;
}

bool UIWizardNewVMNameOSTypeCommon::checkISOFile(UINameAndSystemEditor *pNameAndSystemEditor)
{
    if (!pNameAndSystemEditor)
        return false;
    const QString &strPath = pNameAndSystemEditor->ISOImagePath();
    if (strPath.isNull() || strPath.isEmpty())
        return true;
    QFileInfo fileInfo(strPath);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return false;
    return true;
}

UIWizardNewVMNameOSTypePage::UIWizardNewVMNameOSTypePage()
    : m_pNameAndSystemLayout(0)
    , m_pNameAndSystemEditor(0)
    , m_pSkipUnattendedCheckBox(0)
    , m_pNameOSTypeLabel(0)
    , m_pInfoLabel(0)
{
    prepare();
}

void UIWizardNewVMNameOSTypePage::setISOFilePath(const QString &strISOFilePath)
{
    QFileInfo isoFileInfo(strISOFilePath);
    if (isoFileInfo.exists() && m_pNameAndSystemEditor)
        m_pNameAndSystemEditor->setISOImagePath(strISOFilePath);
}

void UIWizardNewVMNameOSTypePage::prepare()
{
    QVBoxLayout *pPageLayout = new QVBoxLayout(this);
    if (pPageLayout)
    {
        m_pNameOSTypeLabel = new QIRichTextLabel(this);
        if (m_pNameOSTypeLabel)
            pPageLayout->addWidget(m_pNameOSTypeLabel);

        /* Prepare Name and OS Type editor: */
        pPageLayout->addWidget(createNameOSTypeWidgets());

        pPageLayout->addStretch();
    }

    createConnections();
}

void UIWizardNewVMNameOSTypePage::createConnections()
{
    if (m_pNameAndSystemEditor)
    {
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigNameChanged, this, &UIWizardNewVMNameOSTypePage::sltNameChanged);
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigPathChanged, this, &UIWizardNewVMNameOSTypePage::sltPathChanged);
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigOsTypeChanged, this, &UIWizardNewVMNameOSTypePage::sltOsTypeChanged);
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigImageChanged, this, &UIWizardNewVMNameOSTypePage::sltISOPathChanged);
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigOSFamilyChanged, this, &UIWizardNewVMNameOSTypePage::sltGuestOSFamilyChanged);
        connect(m_pNameAndSystemEditor, &UINameAndSystemEditor::sigEditionChanged, this, &UIWizardNewVMNameOSTypePage::sltSelectedEditionChanged);
    }
    if (m_pSkipUnattendedCheckBox)
        connect(m_pSkipUnattendedCheckBox, &QCheckBox::toggled, this, &UIWizardNewVMNameOSTypePage::sltSkipUnattendedInstallChanged);
}

bool UIWizardNewVMNameOSTypePage::isComplete() const
{
    markWidgets();
    if (m_pNameAndSystemEditor->name().isEmpty())
        return false;
    return UIWizardNewVMNameOSTypeCommon::checkISOFile(m_pNameAndSystemEditor);
}

void UIWizardNewVMNameOSTypePage::sltNameChanged(const QString &strNewName)
{
    AssertReturnVoid(wizardWindow<UIWizardNewVM>());
    if (!m_userModifiedParameters.contains("GuestOSTypeFromISO"))
    {
        m_pNameAndSystemEditor->blockSignals(true);
        if (UIWizardNewVMNameOSTypeCommon::guessOSTypeFromName(m_pNameAndSystemEditor, strNewName))
        {
            wizardWindow<UIWizardNewVM>()->setGuestOSType(m_pNameAndSystemEditor->type());
            m_userModifiedParameters << "GuestOSTypeFromName";
        }
        m_pNameAndSystemEditor->blockSignals(false);
    }
    UIWizardNewVMNameOSTypeCommon::composeMachineFilePath(m_pNameAndSystemEditor, wizardWindow<UIWizardNewVM>());
    emit completeChanged();
}

void UIWizardNewVMNameOSTypePage::sltPathChanged(const QString &strNewPath)
{
    Q_UNUSED(strNewPath);
    UIWizardNewVMNameOSTypeCommon::composeMachineFilePath(m_pNameAndSystemEditor, wizardWindow<UIWizardNewVM>());
}

void UIWizardNewVMNameOSTypePage::sltOsTypeChanged()
{
    AssertReturnVoid(wizardWindow<UIWizardNewVM>());
    //m_userModifiedParameters << "GuestOSType";
    if (m_pNameAndSystemEditor)
        wizardWindow<UIWizardNewVM>()->setGuestOSType(m_pNameAndSystemEditor->type());
}

void UIWizardNewVMNameOSTypePage::retranslateUi()
{
    setTitle(UIWizardNewVM::tr("Virtual machine Name and Operating System"));

    if (m_pNameOSTypeLabel)
        m_pNameOSTypeLabel->setText(UIWizardNewVM::tr("Please choose a descriptive name and destination folder for the new "
                                                      "virtual machine. The name you choose will be used throughout VirtualBox "
                                                      "to identify this machine. Additionally, you can select an ISO image which "
                                                      "may be used to install the guest operating system."));

    if (m_pSkipUnattendedCheckBox)
    {
        m_pSkipUnattendedCheckBox->setText(UIWizardNewVM::tr("&Skip Unattended Installation"));
        m_pSkipUnattendedCheckBox->setToolTip(UIWizardNewVM::tr("When checked, the unattended install is disabled and the selected ISO "
                                                                "is mounted on the vm."));
    }

    if (m_pNameAndSystemLayout && m_pNameAndSystemEditor)
        m_pNameAndSystemLayout->setColumnMinimumWidth(0, m_pNameAndSystemEditor->firstColumnWidth());

    updateInfoLabel();
}

void UIWizardNewVMNameOSTypePage::updateInfoLabel()
{
    UIWizardNewVM *pWizard = wizardWindow<UIWizardNewVM>();
    AssertReturnVoid(pWizard);

    if (!m_pInfoLabel)
        return;

    /*
     * Here are the scenarios we consider while updating the info label:
     * - No ISO selected,
     * - Unattended cannot determine OS type from the ISO,
     * - Unattended can determine the OS type from the ISO but cannot install it,
     * - User has disabled unattended explicitly,
     * - Unattended install will kick off.
     */
    QString strMessage;
    if (m_pNameAndSystemEditor->ISOImagePath().isEmpty())
        strMessage = UIWizardNewVM::tr("No ISO image is selected, the guest OS will need to be installed manually.");
    else if (pWizard->detectedOSTypeId().isEmpty())
        strMessage = UIWizardNewVM::tr("OS type cannot be determined from the selected ISO, "
                                       "the guest OS will need to be installed manually.");
    else if (!pWizard->detectedOSTypeId().isEmpty())
    {
        QString strType = uiCommon().vmGuestOSTypeDescription(pWizard->detectedOSTypeId());
        if (!pWizard->isUnattendedInstallSupported())
            strMessage = UIWizardNewVM::tr("Detected OS type: %1. %2")
                                           .arg(strType)
                                           .arg(UIWizardNewVM::tr("This OS type cannot be installed unattendedly. "
                                                                  "The install needs to be started manually."));
        else if (pWizard->skipUnattendedInstall())
            strMessage = UIWizardNewVM::tr("You have selected to skip unattended guest OS install, "
                                           "the guest OS will need to be installed manually.");
        else
            strMessage = UIWizardNewVM::tr("Detected OS type: %1. %2")
                                           .arg(strType)
                                           .arg(UIWizardNewVM::tr("This OS type can be installed unattendedly. "
                                                                  "The install will start after this wizard is closed."));
    }

    m_pInfoLabel->setText(QString("<img src=\":/session_info_16px.png\" style=\"vertical-align:top\"> %1").arg(strMessage));
}

void UIWizardNewVMNameOSTypePage::initializePage()
{
    UIWizardNewVM *pWizard = wizardWindow<UIWizardNewVM>();
    AssertReturnVoid(pWizard);

    retranslateUi();

    /* Initialize this page's widgets etc: */
    {
        if (m_pNameAndSystemEditor)
        {
            m_pNameAndSystemEditor->setFocus();
            setEditionSelectorEnabled();
        }
        setSkipCheckBoxEnable();
    }

    /* Initialize some of the wizard's parameters: */
    {
        if (m_pNameAndSystemEditor)
        {
            pWizard->setGuestOSFamilyId(m_pNameAndSystemEditor->familyId());
            pWizard->setGuestOSType(m_pNameAndSystemEditor->type());
            /* Vm name, folder, file path etc. will be initilized by composeMachineFilePath: */
        }
    }
}

bool UIWizardNewVMNameOSTypePage::validatePage()
{
    /* Try to create machine folder: */
    return UIWizardNewVMNameOSTypeCommon::createMachineFolder(m_pNameAndSystemEditor, wizardWindow<UIWizardNewVM>());
}

void UIWizardNewVMNameOSTypePage::sltISOPathChanged(const QString &strPath)
{
    UIWizardNewVM *pWizard = qobject_cast<UIWizardNewVM*>(this->wizard());
    AssertReturnVoid(pWizard);

    pWizard->setISOFilePath(strPath);

    bool const fOsTypeFixed = UIWizardNewVMNameOSTypeCommon::guessOSTypeDetectedOSTypeString(m_pNameAndSystemEditor,
                                                                                             pWizard->detectedOSTypeId());
    if (fOsTypeFixed)
        m_userModifiedParameters << "GuestOSTypeFromISO";
    else /* Remove GuestOSTypeFromISO from the set if it is there: */
        m_userModifiedParameters.remove("GuestOSTypeFromISO");

    /* Update the global recent ISO path: */
    QFileInfo fileInfo(strPath);
    if (fileInfo.exists() && fileInfo.isReadable())
        uiCommon().updateRecentlyUsedMediumListAndFolder(UIMediumDeviceType_DVD, strPath);

    /* Populate the editions selector: */
    if (m_pNameAndSystemEditor)
         m_pNameAndSystemEditor->setEditionNameAndIndices(pWizard->detectedWindowsImageNames(),
                                                          pWizard->detectedWindowsImageIndices());

    setSkipCheckBoxEnable();
    setEditionSelectorEnabled();
    updateInfoLabel();

    /* Disable OS type selector(s) to prevent user from changing guest OS type manually: */
    if (m_pNameAndSystemEditor)
    {
        m_pNameAndSystemEditor->setOSTypeStuffEnabled(!fOsTypeFixed);

        /* Redetect the OS type using the name if detection or the step above failed: */
        if (!fOsTypeFixed)
            sltNameChanged(m_pNameAndSystemEditor->name());
    }

    emit completeChanged();
}

void UIWizardNewVMNameOSTypePage::sltGuestOSFamilyChanged(const QString &strGuestOSFamilyId)
{
    AssertReturnVoid(wizardWindow<UIWizardNewVM>());
    wizardWindow<UIWizardNewVM>()->setGuestOSFamilyId(strGuestOSFamilyId);
}

void UIWizardNewVMNameOSTypePage::sltSelectedEditionChanged(ulong uEditionIndex)
{
    UIWizardNewVM *pWizard = wizardWindow<UIWizardNewVM>();
    AssertReturnVoid(pWizard);
    pWizard->setSelectedWindowImageIndex(uEditionIndex);
    /* Update the OS type since IUnattended updates the detected OS type after edition (image index) changes: */
    UIWizardNewVMNameOSTypeCommon::guessOSTypeDetectedOSTypeString(m_pNameAndSystemEditor, pWizard->detectedOSTypeId());
}

void UIWizardNewVMNameOSTypePage::sltSkipUnattendedInstallChanged(bool fSkip)
{
    AssertReturnVoid(wizardWindow<UIWizardNewVM>());
    m_userModifiedParameters << "SkipUnattendedInstall";
    wizardWindow<UIWizardNewVM>()->setSkipUnattendedInstall(fSkip);
    setEditionSelectorEnabled();
    updateInfoLabel();
}

QWidget *UIWizardNewVMNameOSTypePage::createNameOSTypeWidgets()
{
    /* Prepare container widget: */
    QWidget *pContainerWidget = new QWidget;
    if (pContainerWidget)
    {
        /* Prepare layout: */
        m_pNameAndSystemLayout = new QGridLayout(pContainerWidget);
        if (m_pNameAndSystemLayout)
        {
            m_pNameAndSystemLayout->setContentsMargins(0, 0, 0, 0);

            /* Prepare Name and OS Type editor: */
            m_pNameAndSystemEditor = new UINameAndSystemEditor(0,
                                                               true /* fChooseName? */,
                                                               true /* fChoosePath? */,
                                                               true /* fChooseImage? */,
                                                               true /* fChooseEdition? */,
                                                               true /* fChooseType? */);
            if (m_pNameAndSystemEditor)
                m_pNameAndSystemLayout->addWidget(m_pNameAndSystemEditor, 0, 0, 1, 2);

            /* Prepare Skip Unattended checkbox: */
            m_pSkipUnattendedCheckBox = new QCheckBox;
            if (m_pSkipUnattendedCheckBox)
                m_pNameAndSystemLayout->addWidget(m_pSkipUnattendedCheckBox, 1, 1);
            m_pInfoLabel = new QIRichTextLabel;
            if (m_pInfoLabel)
                m_pNameAndSystemLayout->addWidget(m_pInfoLabel, 2, 1);
        }
    }

    /* Return container widget: */
    return pContainerWidget;
}

void UIWizardNewVMNameOSTypePage::markWidgets() const
{
    if (m_pNameAndSystemEditor)
    {
        m_pNameAndSystemEditor->markNameEditor(m_pNameAndSystemEditor->name().isEmpty());
        m_pNameAndSystemEditor->markImageEditor(!UIWizardNewVMNameOSTypeCommon::checkISOFile(m_pNameAndSystemEditor),
                                                UIWizardNewVM::tr("Invalid file path or unreadable file"));
    }
}

void UIWizardNewVMNameOSTypePage::setSkipCheckBoxEnable()
{
    AssertReturnVoid(m_pSkipUnattendedCheckBox && m_pNameAndSystemEditor);
    const QString &strPath = m_pNameAndSystemEditor->ISOImagePath();
    if (strPath.isEmpty())
    {
        m_pSkipUnattendedCheckBox->setEnabled(false);
        return;
    }
    if (!isUnattendedInstallSupported())
    {
        m_pSkipUnattendedCheckBox->setEnabled(false);
        return;
    }

    m_pSkipUnattendedCheckBox->setEnabled(UIWizardNewVMNameOSTypeCommon::checkISOFile(m_pNameAndSystemEditor));
}

bool UIWizardNewVMNameOSTypePage::isUnattendedEnabled() const
{
    UIWizardNewVM *pWizard = wizardWindow<UIWizardNewVM>();
    AssertReturn(pWizard, false);
    return pWizard->isUnattendedEnabled();
}

bool UIWizardNewVMNameOSTypePage::isUnattendedInstallSupported() const
{
    UIWizardNewVM *pWizard = wizardWindow<UIWizardNewVM>();
    AssertReturn(pWizard, false);
    return pWizard->isUnattendedInstallSupported();
}


void  UIWizardNewVMNameOSTypePage::setEditionSelectorEnabled()
{
    if (!m_pNameAndSystemEditor || !m_pSkipUnattendedCheckBox)
        return;
    m_pNameAndSystemEditor->setEditionSelectorEnabled(   !m_pNameAndSystemEditor->isEditionsSelectorEmpty()
                                                      && !m_pSkipUnattendedCheckBox->isChecked());
}
