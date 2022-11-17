#!/bin/sh
#
aclocal
automake --foreign --add-missing
autoconf

#./configure $*
echo "Now you are ready to run ./configure"
