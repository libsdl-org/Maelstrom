/* ----------------------------------------------------------------- */
/* -- Special logic module variables */

// in logic.cc :
extern Controls pressed;
extern ULONG	gLastKeyPolled;
extern int	gShipSprite;
extern long	gScore;
extern Sprite  *gSprites[MAX_SPRITES];

// in blit.cc :
extern int	gThrustWasOn;
extern ULONG	gDeadTime;
extern ShotPtr	gTheShots[MAX_SHOTS];
extern ShotPtr	gEnemyShots[MAX_SHOTS];
extern unsigned char *gTheShotColors;
extern unsigned char *gEnemyShotColors;

// in make.cc :
extern ULONG	gPrizeTime;
extern ULONG	gBonusTime;
extern ULONG	gDamagedTime;

// in game.cc :
extern int      gGameOn;
extern int	gNumRoids;
extern ULONG	gBoomDelay;
extern ULONG	gLastBoom;
extern int	gBoomPhase;
extern int	gThrustOn;
extern int	gNumRocks;
extern int	gLives;
extern ULONG	gLastStar;
extern long	gBonus;
extern ULONG	gLastBonus;
extern ULONG	gWaveStarted;
extern ULONG	gWhenDone;
extern ULONG	gOurTicks;

extern int	gMultiplierShown;
extern ULONG	gMultTime;
extern ULONG	gWhenMult;
extern int	gMultFactor;

extern int	gPrizeShown;
extern ULONG	gWhenPrize;

extern int	gBonusShown;
extern ULONG	gWhenBonus;

extern ULONG	gWhenHoming;
extern ULONG	gWhenGrav;
extern ULONG	gWhenDamaged;

extern int	gAirBrakes;
extern int	gAutoFire;
extern int	gLuckOfTheIrish;
extern int	gLongFire;
extern int	gTripleFire;

extern int	gShieldOn;
extern int	gShieldLevel;
extern int	gShaking;
extern ULONG	gWhenNova;
extern ULONG	gShakeTime;
extern int	gNovaBlast;
extern int	gNovaFlag;
extern ULONG	gShieldTime;
extern ULONG	gFreezeTime;
extern ULONG	gEnemyFireTime;

extern int	gWave;
extern int	gEnemySprite;
extern ULONG	gWhenEnemy;

