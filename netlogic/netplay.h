
/* Functions in netplay.cc */
extern int   InitNetData(void);
extern void  HaltNetData(void);
extern int   AddPlayer(char *playerstr);
extern int   SetServer(char *serverstr);
extern int   CheckPlayers(void);
extern void  QueueKey(unsigned char Op, unsigned char Type);
extern int   SyncNetwork(void);
extern int   GetSyncBuf(int index, unsigned char **bufptr);
extern int   Send_NewGame(int *Wave, int *Lives, int *Turbo);
extern int   Await_NewGame(int *Wave, int *Lives, int *Turbo);

/* Variables from netplay.cc */
extern int	gOurPlayer;
extern int	gNumPlayers;
extern int	gDeathMatch;

