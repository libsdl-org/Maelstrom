
/* Game Logic interface routines and variables */

#include "netlogic/netlogic.h"

/* From logic.cpp */
extern void LogicUsage(void);
extern int  InitLogicData(void);
extern int  LogicParseArgs(char ***argvptr, int *argcptr);
extern int  InitLogic(void);
extern int  InitPlayerSprites(void);
extern void HaltLogic(void);
extern void SetControl(unsigned char which, int toggle);
extern int  SpecialKey(SDL_Keycode key);
extern int GetScore(void);

/* From game.cpp */
extern void NewGame(void);

/* From about.cpp */
extern void DoAbout(void);

/* From player.cpp */
extern Uint8 gPlayerShotColors[];
extern SDL_Texture *gPlayerShot;
extern Uint8 gEnemyShotColors[];
extern SDL_Texture *gEnemyShot;

