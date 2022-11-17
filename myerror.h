
#ifndef _myerror_h
#define _myerror_h

/* Generic error message routines */

extern void error(const char *fmt, ...);
extern void mesg(const char *fmt, ...);
extern void myperror(const char *msg);

#endif /* _myerror_h */

