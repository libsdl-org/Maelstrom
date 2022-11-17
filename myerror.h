
#ifndef _myerror_h
#define _myerror_h

/* Generic error message routines */

extern void error(char *fmt, ...);
extern void mesg(char *fmt, ...);
extern void myperror(char *msg);

#define perror(msg)	myperror(msg)

#endif /* _myerror_h */

