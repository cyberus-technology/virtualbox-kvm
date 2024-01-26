#!/bin/sh
## @file
# ???
#

#
# Copyright (C) 2009-2023 Oracle and/or its affiliates.
#
# This file is part of VirtualBox base platform packages, as
# available from https://www.virtualbox.org.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, in version 3 of the
# License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses>.
#
# SPDX-License-Identifier: GPL-3.0-only
#

#change device to select NAT${NAT_STAT_DEVICE} counters

NAT_STAT_DEBUG=1
NAT_STAT_DEVICE=0
NAT_TMP=/tmp
NAT_STATS_FMT=wiki

NAT_STATS_CONVERTER=$NAT_TMP/converter.awk
NAT_STATS_COUNTERS_RAW=${NAT_TMP}/counters.out.raw
NAT_STATS_COUNTERS=${NAT_TMP}/counters.out
NAT_STATS_REPORT=NAT_STATS_NAME.${NAT_STATS_FMT}

NAT_IN_FILE=$1

[ x"$TMP" != x ] && NAT_TMP=TMP

grep NAT${NAT_STAT_DEVICE} $NAT_IN_FILE > $NAT_STATS_COUNTERS_RAW
[ $? -ne 0 ] && echo "error happens while grep'ing the NAT's counters" && exit 1

#sed -ne "s/\ */\t/gp" $NAT_STATS_COUNTERS_RAW > $NAT_STATS_COUNTERS
cp $NAT_STATS_COUNTERS_RAW  $NAT_STATS_COUNTERS

cat > $NAT_STATS_CONVERTER <<EOF
BEGIN{
    if (FMT == "tsv")
        OFS="\t";
    else if (FMT == "wiki")
        OFS="</td><td>"

    FS=" ";
    if (FMT == "wiki")
        print "<table>"
    if (COUNTERS == "counting")
    {
        NF = 2;
        \$1 = "name"
        \$2 = "count"
        if (FMT == "wiki")
            print "<tr><td>" \$0 "</td></tr>"
        else
            print \$0
    }
    else if (COUNTERS == "profiling")
    {
        NF=6
        \$1 = "name"
        \$2 = "ticks_per_count"
        \$3 = "total_ticks"
        \$4 = "times"
        \$5 = "max"
        \$6 = "min"
        if (FMT == "wiki")
            print "<tr><td>" \$0 "</td></tr>"
        else
            print \$0
    }
}
/*counting counters */
NF == 3 && COUNTERS=="counting"{
    name = \$1
    count = \$2
    NF=2
    if (FMT == "wiki")
        print "<tr><td>" \$0 "</td></tr>"
    else
        print \$0
}
/*profiling counters */
NF == 12 && COUNTERS=="profiling"{
    name = \$1
    ticks_per_count = \$2
    total_ticks = \$5
    times = \$7
    max = \$10
    min = \$12
    NF=6
    \$1 = name
    \$2 = ticks_per_count
    \$3 = total_ticks
    \$4 = times
    \$5 = substr(max,0, index(max, ",") -1)
    \$6 = substr(min,0, index(min, ")") - 1)

    if (FMT == "wiki")
        print "<tr><td>" \$0 "</td></tr>"
    else
        print \$0
}
END{
    if (FMT == "wiki")
        print "</table>"
}
EOF
awk -v FMT=$NAT_STATS_FMT -v COUNTERS=profiling -f $NAT_STATS_CONVERTER  $NAT_STATS_COUNTERS > $NAT_STATS_REPORT
awk -v FMT=$NAT_STATS_FMT -v COUNTERS=counting -f $NAT_STATS_CONVERTER  $NAT_STATS_COUNTERS >> $NAT_STATS_REPORT

