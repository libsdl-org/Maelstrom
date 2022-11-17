
#include <sys/time.h>

#ifdef __WIN95__
/* Delay -- delay given in 1/60th of a second */
static inline void Delay(int sixty_th)
{
	Sleep((1000*sixty_th)/60);
}
/* Return current time (in 1/60'th of a second) */
static inline unsigned long Ticks(void)
{
	return((GetTickCount()*60)/1000);
}
#else
/* Delay -- delay given in 1/60th of a second */
static inline void Delay(int sixty_th)
{
	struct timeval tv;

	/* select() is a fairly accurate delay? */
	tv.tv_sec = (sixty_th/60);
	tv.tv_usec = ((sixty_th%60)*(1000000/60));
	(void) select(0, NULL, NULL, NULL, &tv);
}
/* Return current time (in 1/60'th of a second) */
static inline unsigned long Ticks(void)
{
	/* When we started the game */
	extern unsigned long started;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return((tv.tv_sec-started)*60 + (tv.tv_usec*60)/1000000);
}
#endif /* __WIN95__ */

/* The random functions have been moved to fastrand.c */
extern void  SeedRandom(unsigned long seed);
extern short FastRandom(short range);
extern unsigned long GetRandSeed(void);
