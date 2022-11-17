
/* Game Logic interface routines and variables */

#define NETPLAY
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
extern int  SpecialKey(SDL_keysym key);
extern int GetScore(void);

/* From game.cpp */
extern void NewGame(void);

/* From about.cpp */
extern void DoAbout(void);

/* From blit.cpp (fastlogic) player.cpp (netlogic) */
extern Uint8 gPlayerShotColors[];
extern SDL_Surface *gPlayerShot;
extern Uint8 gEnemyShotColors[];
extern SDL_Surface *gEnemyShot;

