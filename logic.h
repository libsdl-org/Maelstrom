
/* Game Logic interface routines and variables */

#ifdef NETPLAY
#include "netlogic/netlogic.h"
#else
#include "fastlogic/fastlogic.h"
#endif

/* From logic.cpp */
extern void LogicUsage(void);
extern void InitLogicData(void);
extern int  LogicParseArgs(char ***argvptr, int *argcptr);
extern int  InitLogic(void);
extern int  InitPlayerSprites(void);
extern void HaltLogic(void);
extern void SetControl(unsigned char which, int toggle);
extern int  SpecialKey(KeySym key);
extern int GetScore(void);

/* From game.cpp */
extern void NewGame(void);

/* From about.cpp */
extern void DoAbout(void);

/* From blit.cpp (fastlogic) player.cpp (netlogic) */
extern unsigned char *gPlayerShotColors;
extern unsigned char *gEnemyShotColors;
extern unsigned char gShotMask[SHOT_SIZE*SHOT_SIZE];

