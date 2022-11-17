
/* Used by both Maelstrom and the sound server for Maelstrom */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef __WIN95__
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/file.h>			/* For FASYNC on Solaris */
#include <sys/ioctl.h>			/* For FIOASYNC on HPUX */
#endif

#ifdef TESTING
#ifndef LIBDIR
#ifdef __WIN95__
#define LIBDIR ".."
#else
#define LIBDIR	"."
#endif /* Win95 */
#endif
#else
#ifndef LIBDIR
#ifdef __WIN95__
#define LIBDIR "."
#else
#define LIBDIR	"/usr/local/lib/Maelstrom"
#endif /* Win95 */
#endif
#endif /* TESTING */

char *file2libpath(char *filename)
{
	char *libdir;
	static char fullpath[256];

	if ( (libdir=getenv("MAELSTROM_LIB")) == NULL )
		libdir=LIBDIR;
	sprintf(fullpath, "%s/%s", libdir, filename);
	return(fullpath);
}

#ifndef __WIN95__
/* This function written by Tom Anderson (tom@proximity.com.au) */
void select_usleep(unsigned long usec)
{
    struct timeval tv;

    if(usec > 1000000L) {	// don't do expensive /, % unless we need to
	tv.tv_sec  = usec / 1000000L;
	tv.tv_usec = usec % 1000000L;
    } else {
	tv.tv_sec  = 0;
	tv.tv_usec = usec;
    }
    (void) select(0, NULL, NULL, NULL, &tv);
}

/* A routine to set up asynchronous I/O on a file descriptor.
   Note that an error in this function is fatal.
*/
int Set_AsyncIO(int fd, void (*handler)(int sig), long handler_flags)
{
        struct sigaction action;
        long  flags;

	/* Set up the I/O handler */
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
#ifdef _INCLUDE_HPUX_SOURCE
        action.sa_flags   = 0;
#else
	action.sa_flags   = handler_flags;
#endif
	sigaction(SIGIO, &action, NULL);
#ifdef _INCLUDE_HPUX_SOURCE
	flags = 1;
	if ( ioctl(fd, FIOASYNC, &flags) < 0 ) {
		perror(
                "Set_AsyncIO: Can't set asynchronous I/O on socket");
		exit(255);
	}
	flags = getpid();
	if ( ioctl(fd, SIOCSPGRP, &flags) < 0 ) {
		perror(
                "Set_AsyncIO: Can't set process group for socket");
		exit(255);
	}
#else /* linux, SGI, sparc, etc */
	flags = fcntl(fd, F_GETFL, 0);
	flags |= FASYNC;
	if ( fcntl(fd, F_SETFL, flags) < 0 ) {
		perror(
                "Set_AsyncIO: Can't set asynchronous I/O on socket");
		exit(255);
	}
	if ( fcntl(fd, F_SETOWN, getpid()) < 0 ) {
		perror(
                "Set_AsyncIO: Can't set process group for socket");
		exit(255);
	}
#endif
	return(0);
}

#endif /* ! Win95 */
