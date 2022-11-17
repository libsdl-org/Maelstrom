
/* A program to test asynchronous I/O on a system */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifndef SA_NOMASK       /* sparc */
#define SA_NOMASK       SA_NODEFER
#endif

char *id; int fd;
void IO_Handler(int sig)
{
	char buf[54]; int len;

	fprintf(stderr, "%s: I/O!\n", id);
	len=read(fd, buf, 54);
	write(1, buf, len);
	write(fd, "Reply..\n", 8);
	exit(0);
}

int Set_AsyncIO(int fd)
{
        struct sigaction action;
        long  flags;

                /* Set up the I/O handler (allow interrupts during handler) */
                action.sa_handler = IO_Handler;
                sigemptyset(&action.sa_mask);
#if defined(_INCLUDE_HPUX_SOURCE) || defined __mips
                action.sa_flags   = 0;
#else
#ifdef SA_INTERRUPT	/* SA_RESTART is the default (SunOS 4.1.4) */
                action.sa_flags   = 0;
#else			/* We need to specify SA_RESTART */
                action.sa_flags   = SA_RESTART;
#endif /* SA_INTERRUPT */
#endif /* _INCLUDE_HPUX_SOURCE */
                sigaction(SIGIO, &action, NULL);
#ifdef _INCLUDE_HPUX_SOURCE
                flags = 1;
                if ( ioctl(fd, FIOASYNC, &flags) < 0 ) {
                        perror(
                "SoundClient: Can't set asynchronous I/O on socket");
                        exit(255);
                }
                flags = getpid();
                if ( ioctl(fd, SIOCSPGRP, &flags) < 0 ) {
                        perror(
                "SoundClient: Can't set process group for socket");
                        exit(255);
                }
#else /* linux, SGI, sparc, etc */
                flags = fcntl(fd, F_GETFL, 0);
                flags |= FASYNC;
                if ( fcntl(fd, F_SETFL, flags) < 0 ) {
                        perror(
                "SoundClient: Can't set asynchronous I/O on socket");
                        exit(255);
                }
                if ( fcntl(fd, F_SETOWN, getpid()) < 0 ) {
                        perror(
                "SoundClient: Can't set process group for socket");
                        exit(255);
                }
#endif
		return(0);
}

main(int argc, char *argv[])
{
	extern char *optarg;
	int stream_fds[2];

	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, stream_fds) < 0 ) {
                perror("SoundClient: Can't create stream sockets");
                exit(255);
        }

	switch (fork()) {
		case 0:	/* Child */
			id="Child";
			fd=stream_fds[0];
			Set_AsyncIO(fd);
			pause();
		default:
			id="Parent";
			fd=stream_fds[1];
			Set_AsyncIO(fd);
			sleep(1);
			write(fd, "Hi there!\n", 10);
			pause();
	}
	fprintf(stderr, "%s: broke out of switch!\n", id);
}
