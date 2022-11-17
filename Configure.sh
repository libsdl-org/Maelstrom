#!/bin/sh
#
# This script goes through the intense process of configuring Maelstrom

# The query script
Ask="sh ./Ask.sh"

# Set some global defaults
audio_ext="std"
bindir="/usr/local/bin"
libdir="/usr/local/lib/Maelstrom"
cxx="g++"
x11libs='-lXpm $(XPMLINKDIR) -lX11 -lXext $(X11LINKDIR)'
# Profiling:
#optimize="-pg"
# Full optimization with g++:
optimize="-O6 -fomit-frame-pointer"
# Debugging:
#optimize="-g"

# Figure out what kind of system we are on:
system=`uname`
. ./SetDefault.sh $system

# Get the installation program
if [ -f /usr/bin/X11/bsdinst ]; then
	install="/usr/bin/X11/bsdinst"
elif [ -f /usr/ucb/install ]; then
	install="/usr/ucb/install"
else
	install="install"
fi

# Get some additional extra features
if [ "`$Ask 'Do you want to force X11 shared memory?' 'n'`" != "n" ]; then
	features="$features -DFORCE_XSHM"
fi
if [ "`$Ask 'Do you want 1280x960 X11 resolution? (slow!)' 'n'`" != "n" ]; then
	features="$features -DPIXEL_DOUBLING"
	echo " - Interlaced pixel doubling is faster, but not as bright."
	if [ "`$Ask ' Do you want interlaced pixel doubling?' 'n'`" != "n" ];
	then features="$features -DINTERLACE_DOUBLED"
	fi
fi
if [ "`$Ask 'Do you want to compile multi-player version?' 'y'`" = "y" ]; then
	features="$features -DNETPLAY"
	logic="netlogic"
else
	logic="fastlogic"
fi
# Check for RSA library
if [ -d ../RSA ]; then
	features="$features -DUSE_CHECKSUM"
	csumlib="../RSA/install/rsaref.a"
fi

# Check the compiler
cxx=`$Ask 'What compiler do you want to use?' "$cxx"`
optimize=`$Ask 'What optimization flags do you want to use?' "$optimize"`

# Check for XPM libraries
cwd=`pwd`
if [ -d "../xpm-3.4g" ]; then
	parent=`dirname $cwd`
	xpminclude="-I${parent}/xpm-3.4g/include"
	xpmlinkdir="-L${parent}/xpm-3.4g/lib"
fi

# Find out where to install Maelstrom
bindir=`$Ask 'Where do you want to install binaries?' "$bindir"`
libdir=`$Ask 'Where do you want data files installed?' "$libdir"`

# Set up the proper audio driver
if [ -f "mixer-$audio_ext.h" ] && [ -f "mixer-$audio_ext.cpp" ]; then
	ln -sf mixer-$audio_ext.h mixer.h 2>/dev/null || \
		cp mixer-$audio_ext.h mixer.h
	ln -sf mixer-$audio_ext.cpp mixer.cpp 2>/dev/null || \
		cp mixer-$audio_ext.cpp mixer.cpp
fi

# Create a new Makeflags file
echo ""
echo "Creating new Makeflags..."
sed <Makeflags.in >Makeflags \
	-e "s|^\\(SYSTEM =\\)\$|\\1 $system|" \
	-e "s|^\\(FEATURES =\\)\$|\\1 $features|" \
	-e "s|^\\(LOGIC =\\)\$|\\1 $logic|" \
	-e "s|^\\(XPMINCLUDE =\\)\$|\\1 $xpminclude|" \
	-e "s|^\\(XPMLINKDIR =\\)\$|\\1 $xpmlinkdir|" \
	-e "s|^\\(BINDIR =\\)\$|\\1 $bindir|" \
	-e "s|^\\(LIBDIR =\\)\$|\\1 $libdir|" \
	-e "s|^\\(CXX =\\)\$|\\1 $cxx|" \
	-e "s|^\\(OPTIMIZE =\\)\$|\\1 $optimize|" \
	-e "s|^\\(X11INCLUDE =\\)\$|\\1 $x11include|" \
	-e "s|^\\(EXTRALIBS =\\)\$|\\1 $extralibs|" \
	-e "s|^\\(X11LINKDIR =\\)\$|\\1 $x11linkdir|" \
	-e "s|^\\(X11LIBS =\\)\$|\\1 $x11libs|" \
	-e "s|^\\(CSUMLIB =\\)\$|\\1 $csumlib|" \
	-e "s|^\\(SOUNDLIBS =\\)\$|\\1 $soundlibs|" \
	-e "s|^\\(INSTALL =\\)\$|\\1 $install|"
echo ""
echo "Now type 'make' to build Maelstrom"
