#!/bin/sh
#
# Set some defaults, based on system type  (not meant to be executed)

#SetDefault()
#{
#	system=$1
	echo ""
	echo "Ahh, I see you are running $system"
	echo ""

	case $system in
		Linux)	
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS"
			machine="`uname -m`"
			if echo $machine | grep 86 >/dev/null; then
				if [ \
"`$Ask 'Do you want X11 Direct Graphics Access (experimental)?' 'n'`" != "n" ]
				then
					features="$features -DUSE_DGA"
					x11libs="-lXxf86dga -lXxf86vm $x11libs"
				fi
				if [ \
"`$Ask 'Do you want console (SVGAlib) support?' 'y'`" = "y" ]
				then
					features="$features -DUSE_SVGALIB"
					extralibs="$extralibs -lvgagl -lvga"
				fi
				if [ \
"`$Ask 'Do you want joystick support?  (See README.joystick)' 'n'`" != "n" ]
				then
					features="$features -DUSE_JOYSTICK"
				fi
# DMA support appears to be broken in newer kernels (2.0.x) (?)
#				if [ \
#"`$Ask 'Do you want DMA audio support (experimental)?' 'n'`" != "n" ]
#				then
#					audio_ext="dma"
#				fi
			fi
			if [ -d "/usr/X11R6/lib" ]; then
				x11linkdir="-L/usr/X11R6/lib"
				x11include="-I/usr/X11R6/include"
			fi
			;;
		FreeBSD)	
# FreeBSD patch submitted by Stephen Hocking <sysseh@devetir.qld.gov.au>
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS"
			if [ \
"`$Ask 'Do you want X11 Direct Graphics Access (experimental)?' 'n'`" != "n" ]
			then
				features="$features -DUSE_DGA"
				x11libs="-lXxf86dga -lXxf86vm $x11libs"
			fi
			if [ \
"`$Ask 'Do you want DMA audio support (experimental)?' 'n'`" != "n" ]
			then
				audio_ext="dma"
			elif [ \
"`$Ask 'Do you want NAS audio support (experimental)?' 'n'`" != "n" ]
			then
				audio_ext="nas"
				soundlibs="-L/usr/X11R6/lib -laudio -lX11"
			else
				features="$features -DAUDIO_16BIT"
			fi
			if [ -d "/usr/X11R6/lib" ]; then
				x11linkdir="-L/usr/X11R6/lib"
				x11include="-I/usr/X11R6/include"
			fi
			;;
		SunOS)	
			# Are we SunOS (4.x) or Solaris (5.x)?
			solaris=`uname -sr | fgrep "SunOS 5"`
			if [ "$solaris" != "" ]; then
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DPLAY_DEV_AUDIO"
x11include="-I/usr/openwin/include"
extralibs="-lsocket -lnsl"
soundlibs="-lsocket"
system="Solaris"
echo "-- Configuring for $system"
			else # SunOS
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DPLAY_DEV_AUDIO"
x11include="-I/usr/openwin/include"
			fi
			;;
		ULTRIX)
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS"
			;;
		AIX)
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS"
extralibs="-lXextSHM"
			;;
		HP-UX)	
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DAUDIO_16BIT"
echo "**** Note: Sound doesn't work very well on HPUX"
			;;
		IRIX)	
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DAUDIO_16BIT"
cxx="CC"
optimize="-woff 3252,3106"
soundlibs="-laudio"
			;;
		IRIX64)	# Duplicate the IRIX entry
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DAUDIO_16BIT"
cxx="CC"
optimize="-woff 3252,3106"
soundlibs="-laudio"
			;;
		UNIX_SV)	
# SVR4.2 patch submitted by Stephen Hocking <sysseh@devetir.qld.gov.au>
#
# includes hackery with -D_STYPES needed for stopping static copies of lstat
# clashing left right & centre
#
features=" -DASYNCHRONOUS_IO -DUSE_POSIX_SIGNALS -DAUDIO_16BIT"
extralibs="-lsocket -lnsl"
			if [ \
"`$Ask 'Do you want NAS audio support (experimental)?' 'y'`" != "n" ]
			then
				audio_ext="nas"
			else
				features="$features -DAUDIO_16BIT"
			fi
			if [ -d "/usr/X11R6/lib" ]; then
				x11linkdir="-L/usr/X11R6/lib"
				x11include="-I/usr/X11R6/include -D_STYPES"
				soundlibs="-L/usr/X11R6/lib -laudio -lX11 -lnsl -lsocket"
			else
				x11linkdir="-L/usr/X/lib"
				x11include="-I/usr/X/include -D_STYPES"
				soundlibs="-L/usr/X/lib -laudio -lX11 -lnsl -lsocket"
			fi
			;;
		*)	echo "I don't know how to configure for $system, sorry."
			exit 1
			;;
	esac
#}
