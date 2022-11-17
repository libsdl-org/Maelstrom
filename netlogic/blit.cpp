
/* Note well:  The order that things are done is very important to prevent
               bugs.  Several optimizing assumptions are done in player.cc
               and object.cc that require this order.  For example, I assume
               that all sprites are erased when object->Move() is called,
               that object->HitBy() is called before the move, and that
               object->Move() is called EVERY timestep!
*/

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "globals.h"

/* Returns the number of players left in the game */
int RunFrame(void)
{
	int i, j, PlayersLeft;

	/* Read in keyboard input for our ship */
	HandleEvents(0);

	/* Send Sync! signal to all players, and handle keyboard. */
	if ( SyncNetwork() < 0 ) {
		error("Game aborted!\n");
		return(0);
	}
	OBJ_LOOP(i, gNumPlayers)
		gPlayers[i]->HandleKeys();

	if ( gPaused > 0 )
		return(1);

	/* Play the boom sounds */
	if ( --gNextBoom == 0 ) {
		if ( gBoomPhase ) {
			sound->PlaySound(gBoom1, 0);
			gBoomPhase = 0;
		} else {
			sound->PlaySound(gBoom2, 0);
			gBoomPhase = 1;
		}
		gNextBoom = gBoomDelay;
	}

	/* UnBlit all of the sprites and players */
	OBJ_LOOP(i, gNumPlayers)
		gPlayers[i]->UnBlitSprite();
	OBJ_LOOP(i, gNumSprites)
		gSprites[i]->UnBlitSprite();

	/* Do all hit detection */
	OBJ_LOOP(j, gNumPlayers) {
		if ( ! gPlayers[j]->Alive() )
			continue;

		/* This loop looks funny because gNumSprites can change 
		   dynamically during the loop as sprites are killed/created.
		   This same logic is used whenever looping where sprites
		   might be destroyed.
		*/
		OBJ_LOOP(i, gNumSprites) {
			if ( gSprites[i]->HitBy(gPlayers[j]) < 0 ) {
				delete gSprites[i];
				gSprites[i] = gSprites[gNumSprites];
			}
		}
		OBJ_LOOP(i, gNumPlayers) {
			if ( i == j )	// Don't shoot ourselves. :)
				continue;
			(void) gPlayers[i]->HitBy(gPlayers[j]);
		}
	}
	if ( gEnemySprite ) {
		OBJ_LOOP(i, gNumPlayers) {
			if ( ! gPlayers[i]->Alive() )
				continue;
			(void) gPlayers[i]->HitBy(gEnemySprite);
		}
		OBJ_LOOP(i, gNumSprites) {
			if ( gSprites[i] == gEnemySprite )
				continue;
			if ( gSprites[i]->HitBy(gEnemySprite) < 0 ) {
				delete gSprites[i];
				gSprites[i] = gSprites[gNumSprites];
			}
		}
	}

	/* Handle all the shimmy and the shake. :-) */
	if ( gShakeTime && (gShakeTime-- > 0) ) {
		int shakeV;

		OBJ_LOOP(i, gNumPlayers) {
			shakeV = FastRandom(SHAKE_FACTOR);
			if ( ! gPlayers[i]->Alive() )
				continue;
			gPlayers[i]->Shake(FastRandom(SHAKE_FACTOR));
		}
		OBJ_LOOP(i, gNumSprites) {
			shakeV = FastRandom(SHAKE_FACTOR);
			gSprites[i]->Shake(FastRandom(SHAKE_FACTOR));
		}
	}

	/* Move all of the sprites */
	OBJ_LOOP(i, gNumPlayers)
		gPlayers[i]->Move(0);
	OBJ_LOOP(i, gNumSprites) {
		if ( gSprites[i]->Move(gFreezeTime) < 0 ) {
			delete gSprites[i];
			gSprites[i] = gSprites[gNumSprites];
		}
	}
	if ( gFreezeTime )
		--gFreezeTime;

	/* Now Blit them all again */
	OBJ_LOOP(i, gNumSprites)
		gSprites[i]->BlitSprite();
	OBJ_LOOP(i, gNumPlayers)
		gPlayers[i]->BlitSprite();
	screen->Update();

	/* Make sure someone is still playing... */
	for ( i=0, PlayersLeft=0; i < gNumPlayers; ++i ) {
		if ( gPlayers[i]->Kicking() )
			++PlayersLeft;
	}
	if ( gNumPlayers > 1 ) {
		OBJ_LOOP(i, gNumPlayers)
			gPlayers[i]->ShowDot();
		screen->Update();
	}

#ifdef SERIOUS_DEBUG
printf("Player listing: ");
OBJ_LOOP(i, gNumPlayers) {
	int x, y;
	gPlayers[i]->GetPos(&x, &y);
	printf("  %d = (%d,%d)", i, x, y);
}
printf("\n");

printf("Object listing: ");
OBJ_LOOP(i, gNumSprites) {
	int x, y;
	gSprites[i]->GetPos(&x, &y);
	printf("  %d = (%d,%d)", i, x, y);
}
printf("\n");
#endif /* SERIOUS_DEBUG */

	/* Timing handling -- Delay the FRAME_DELAY */
	screen->Update();
	if ( ! gNoDelay ) {
		Uint32 ticks;
		while ( ((ticks=Ticks)-gLastDrawn) < FRAME_DELAY ) {
			SDL_Delay(1);
		}
		gLastDrawn = ticks;
	}
	return(PlayersLeft);
}
