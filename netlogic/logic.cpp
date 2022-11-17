
#include "Maelstrom_Globals.h"
#include "object.h"
#include "player.h"
#include "globals.h"
#include "netplay.h"

/* Extra options specific to this logic module */
void LogicUsage(void)
{
	error(
"	-player N[@host][:port]	# Designate player N (at host and/or port)\n"
"	-server N@host[:port]	# Play with N players using server at host\n"
"	-deathmatch [N]		# Play deathmatch to N frags (default = 8)\n"
	);
}

/* Initialize special logic data */
int InitLogicData(void)
{
	/* Initialize network player data */
	if ( InitNetData() < 0 ) {
		return(-1);
	}
	gDeathMatch = 0;
	return(0);
}

/* Parse logic-specific command line arguments */
int LogicParseArgs(char ***argvptr, int *argcptr)
{
	char **argv = *argvptr;
	int    argc = *argcptr;

	/* Check for the '-player' option */
	if ( strcmp(argv[1], "-player") == 0 ) {
		if ( ! argv[2] ) {
			error(
			"The '-player' option requires an argument!\n");
			PrintUsage();
		}
		if ( AddPlayer(argv[2]) < 0 )
			exit(1);
		++(*argvptr);
		--(*argcptr);
		return(0);
	}

	/* Check for the '-server' option */
	if ( strcmp(argv[1], "-server") == 0 ) {
		if ( ! argv[2] ) {
			error("The '-server' option requires an argument!\n");
			PrintUsage();
		}
		if ( SetServer(argv[2]) < 0 )
			exit(1);
		++(*argvptr);
		--(*argcptr);
		return(0);
	}

	/* Check for the '-deathmatch' option */
	if ( strcmp(argv[1], "-deathmatch") == 0 ) {
		if ( argv[2] && ((gDeathMatch=atoi(argv[2])) > 0) ) {
			++(*argvptr);
			--(*argcptr);
		} else
			gDeathMatch = 8;
		return(0);
	}
	return(-1);
}

/* Do final logic initialization */
int InitLogic(void)
{
	/* Make sure we have a valid player list */
	if ( CheckPlayers() < 0 )
		return(-1);
	return(0);
}

void HaltLogic(void)
{
	HaltNetData();
}

/* Initialize the player sprites */
int InitPlayerSprites(void)
{
	int index;

	OBJ_LOOP(index, gNumPlayers)
		gPlayers[index] = new Player(index);
	return(0);
}

int SpecialKey(SDL_keysym key)
{
	if ( key.sym == SDLK_F1 ) {
		/* Special key -- switch displayed player */
		if ( ++gDisplayed == gNumPlayers )
			gDisplayed = 0;
		return(0);
	}
	return(-1);
}

void SetControl(unsigned char which, int toggle)
{
	QueueKey(toggle ? KEY_PRESS : KEY_RELEASE, which);
}

int GetScore(void)
{
	return(OurShip->GetScore());
}

