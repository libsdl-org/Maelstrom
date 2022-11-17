
#include "Maelstrom_Globals.h"
#include "object.h"
#include "load.h"

#define	ICON_V_OFF	14

#define KEYPRESS(X)	(X.type == SDL_KEYDOWN)
#define BUTTONPRESS(X)	(X.type == SDL_MOUSEBUTTONDOWN)

/* ----------------------------------------------------------------- */
/* -- Tell 'em about the game */

void DoAbout(void)
{
	SDL_Event event;
	Uint32 clr, ltClr, ltrClr;
	Bool done = false;
	int idOn = 133;
	Bool drawscreen = true;
	int numsprites=0, i;
	Object *objects[MAX_SPRITES];

	/* Set up the colors */
	clr = screen->MapRGB(30000>>8, 30000>>8, 0xFF);
	ltClr = screen->MapRGB(40000>>8, 40000>>8, 0xFF);
	ltrClr = screen->MapRGB(50000>>8, 50000>>8, 0xFF);

	while ( ! done ) {
		int    sound_to_play = 0;

		/* Rotate any sprites */
		for ( i=0; i<numsprites; ++i ) {
			objects[i]->UnBlitSprite();
			objects[i]->Move(0);
			objects[i]->BlitSprite();
		}
		screen->Update();

		/* Wait for keyboard input */
		while ( SDL_PollEvent(&event) ) {
		
			if ( KEYPRESS(event) || BUTTONPRESS(event) ) {
				Bool next_screen;

				next_screen = (
					(event.type == SDL_MOUSEBUTTONDOWN) ||
					(event.key.keysym.sym == SDLK_RETURN)
					);
				drawscreen = true;
			
				if ( next_screen ) {
					sound_to_play = gExplosionSound;
				} else {
					done = true;
					sound_to_play = gMultiplierGone;
				}
				
				if ( ++idOn > 135 ) {
					done = true;
					sound_to_play = gPrettyGood;
				}
				Delay(SOUND_DELAY);
				sound->PlaySound(sound_to_play, 5);

				for ( i=0; i<numsprites; ++i )
					delete objects[i];
				numsprites = 0;
			}
		}
		Delay(1);

		/* -- Handle updates */
		if ( drawscreen && !done ) {
			SDL_Surface *title;
			int   width, height;
			int   xOff,  yOff;

			screen->Fade();
			screen->Clear();

			/* -- Draw the screen frame */
			width = 512;
			height = 384;
			xOff = ((gScrnRect.right - gScrnRect.left) - width) /2;
			yOff = ((gScrnRect.bottom - gScrnRect.top) - height)/2;

			screen->DrawRect(xOff-1,yOff-1,width+2,height+2,clr);
			screen->DrawRect(xOff-2,yOff-2,width+4,height+4,clr);
			screen->DrawRect(xOff-3,yOff-3,width+6,height+6,ltClr);
			screen->DrawRect(xOff-4,yOff-4,width+8,height+8,ltClr);
			screen->DrawRect(xOff-5,yOff-5,width+10,height+10,ltrClr);
			screen->DrawRect(xOff-6,yOff-6,width+12,height+12,ltClr);
			screen->DrawRect(xOff-7,yOff-7,width+14,height+14,clr);
			screen->DrawRect(xOff,yOff,width,height,ltClr);

			/* -- Now draw the picture */
			title = Load_Title(screen, idOn);
			if ( title == NULL ) {
				error(
				"Can't load 'about' title! (ID=%d)\n", idOn);
               			exit(255);
			}
			screen->QueueBlit(xOff, yOff, title, NOCLIP);
			screen->Update();
			screen->FreeImage(title);

			/* Draw color icons if this is Game screen */
			if ( idOn == 134 ) {
				int x, y, off;

				x = (80) * SCALE_FACTOR;
				y = (136) * SCALE_FACTOR;
				off = 39 * SCALE_FACTOR;

				objects[numsprites++] = 
					new Object(x, y, 0, 0, gPlayerShip, 1);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gPrize, 2);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gBonusBlit, 2);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gMult[3], 1);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gDamagedShip, 1);
				y += off;

				/* -- Now for the second column */
				x = (340) * SCALE_FACTOR;
				y = (136) * SCALE_FACTOR;
				off = 39 * SCALE_FACTOR;

				objects[numsprites++] = 
					new Object(x, y, 0, 0, gRock1R, 1);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gSteelRoidR, 1);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gNova, 4);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gMineBlitL, 1);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gVortexBlit, 3);
				y += off;
				objects[numsprites++] = 
					new Object(x, y, 0, 0, gEnemyShip, 1);
				y += off;

				/* Now for the icons */
				x = xOff+25;
				y = yOff+314;
				screen->QueueBlit(x, y, gShieldIcon, NOCLIP);
				screen->QueueBlit(x+16, y,
						gAirBrakesIcon, NOCLIP);
				y += ICON_V_OFF;
				screen->QueueBlit(x, y, gLongFireIcon, NOCLIP);
				screen->QueueBlit(x+16, y,
						gTripleFireIcon, NOCLIP);
				y += ICON_V_OFF;
				screen->QueueBlit(x, y, gAutoFireIcon, NOCLIP);
				screen->QueueBlit(x+16, y,
						gLuckOfTheIrishIcon, NOCLIP);

			}
			if ( idOn == 135 ) {
				MFont *font;
				SDL_Surface *text1, *text2;

				/* Put in the right credits / mask the old... */
				clr = screen->MapRGB(0x00, 0x00, 0x00);
				screen->FillRect(xOff+166,yOff+282,338,62,clr);
				font = fontserv->NewFont("New York", 18);
				if ( font == NULL ) {
					error(
				"Can't use New York(18) font! -- Exiting.\n");
					exit(255);
				}
				text1 = fontserv->TextImage("Port to Linux:   ",
					font, STYLE_NORM, 0xFF, 0xFF, 0x55);
				text2 = fontserv->TextImage("Sam Lantinga",
					font, STYLE_NORM, 0xFF, 0xFF, 0xFF);
				screen->QueueBlit(xOff+178, yOff+298,
								text1, NOCLIP);
				screen->QueueBlit(xOff+178+text1->w, yOff+298,
								text2, NOCLIP);
				fontserv->FreeText(text1);
				fontserv->FreeText(text2);
				delete font;
			}
			screen->Update();
			screen->Fade();
			drawscreen = false;
		}
	}
	screen->Fade();
	gUpdateBuffer = true;
}	/* -- DoAbout */
