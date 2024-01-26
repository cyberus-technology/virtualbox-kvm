#!/bin/sh
set -x
libtoolize --force || exit 1
aclocal || exit 1
autoheader || exit 1
automake --add-missing -c || exit 1
autoconf || exit 1
