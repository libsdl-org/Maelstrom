
#include "Maelstrom_Globals.h"
#include "object.h"

#define	ICON_V_OFF	14


/* ----------------------------------------------------------------- */
/* -- Tell 'em about the game */

void DoAbout(void)
{
	unsigned long clr, ltClr, ltrClr;
	Bool          done = false;
	int           idOn = 133;
	Bool          clear = true;
	Bool          drawscreen = true;
	int           numsprites=0, i;
	Object       *objects[MAX_SPRITES];

	/* Set up the colors */
	clr = win->Map_Color(30000, 30000, 0xFFFF);
	ltClr = win->Map_Color(40000, 40000, 0xFFFF);
	ltrClr = win->Map_Color(50000, 50000, 0xFFFF);

	while ( ! done ) {
		XEvent event;
		char   buf[128];
		KeySym key;
		int    sound_to_play = 0;

		/* Rotate any sprites */
		for ( i=0; i<numsprites; ++i ) {
			objects[i]->UnBlitSprite();
			objects[i]->Move(0);
			objects[i]->BlitSprite();
		}

		/* Wait for keyboard input */
		if ( PollEvent(&event, 1) ) {
		
			/* -- Handle keyboard events */
			if ( (event.type == KeyPress) || 
						(event.type == ButtonPress) ) {
				if ( event.type == KeyPress ) {
					/* Clear state info */
					event.xkey.state = 0;
					win->KeyToAscii(&event, buf, 127, &key);
				}
				clear = false;
				drawscreen = true;
			
				if (++idOn >135)
					done = true;

				if ( (event.type == ButtonPress) ||
					  		(key == XK_Return) ) {
					sound_to_play = gExplosionSound;

					if (idOn == 134) {
					}
					
				} else {
					done = true;
					sound_to_play = gMultiplierGone;
				}
				
				if ( idOn > 135 )
					sound_to_play = gPrettyGood;
				
				if ( sound_to_play ) {
					Delay(SOUND_DELAY);
					sound->PlaySound(sound_to_play,5,NULL);
				}
				for ( i=0; i<numsprites; ++i )
					delete objects[i];
				numsprites = 0;
			}

			/* -- Handle updates */
			if ( drawscreen && !done ) {
				Title title;
				int   width, height;
				int   xOff,  yOff;

				win->Fade(FADE_STEPS);
				win->Clear();

				/* -- Draw the screen frame */
				width = 512;
				height = 384;
				xOff = ((gScrnRect.right - gScrnRect.left) - 
								width) / 2;
				yOff = ((gScrnRect.bottom - gScrnRect.top) - 
								height) /2;
				
				win->DrawRectangle(xOff-1, yOff-1, width+2, 
							height+2, clr);
				win->DrawRectangle(xOff-2, yOff-2, width+4, 
							height+4, clr);
				win->DrawRectangle(xOff-3, yOff-3, width+6, 
							height+6, ltClr);
				win->DrawRectangle(xOff-4, yOff-4, width+8, 
							height+8, ltClr);
				win->DrawRectangle(xOff-5, yOff-5, width+10, 
							height+10, ltrClr);
				win->DrawRectangle(xOff-6, yOff-6, width+12, 
							height+12, ltClr);
				win->DrawRectangle(xOff-7, yOff-7, width+14, 
							height+14, clr);
				win->DrawRectangle(xOff, yOff, width, 
							height, ltClr);

				/* -- Now draw the picture */
				if ( Load_Title(&title, idOn) < 0 ) {
					error(
				"Can't load 'about' title! (ID=%d)\n", idOn);
                			exit(255);
				}
				win->Blit_Title(xOff, yOff, title.width, 
						title.height, title.data);
				Free_Title(&title);

				/* Draw color icons if this is Game screen */
				if ( idOn == 134 ) {
					int	x, y, off;

					x = (80) * SCALE_FACTOR;
					y = (136) * SCALE_FACTOR;
					off = 39 * SCALE_FACTOR;

					objects[numsprites++] = 
						new Object(x,y,0,0,gPlayerShip,1);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gPrize,2);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gBonusBlit,2);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gMult[3],1);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gDamagedShip,1);
					y += off;

					/* -- Now for the second column */
					x = (340) * SCALE_FACTOR;
					y = (136) * SCALE_FACTOR;
					off = 39 * SCALE_FACTOR;

					objects[numsprites++] = 
						new Object(x,y,0,0,gRock1R,1);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gSteelRoidR,1);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gNova,4);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gMineBlitL,1);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gVortexBlit,3);
					y += off;
					objects[numsprites++] = 
						new Object(x,y,0,0,gEnemyShip,1);
					y += off;

					BlitCIcon(xOff+25,yOff+314,gShieldIcon);
					BlitCIcon(xOff+25+16,yOff+314, gAirBrakesIcon);
					BlitCIcon(xOff+25,yOff+314+ICON_V_OFF, gLongFireIcon);
					BlitCIcon(xOff+25+16,yOff+314+ICON_V_OFF, gTripleFireIcon);
					BlitCIcon(xOff+25,yOff+314+(2*ICON_V_OFF), gAutoFireIcon);
					BlitCIcon(xOff+25+16,yOff+314+(2*ICON_V_OFF), gLuckOfTheIrishIcon);

					clear = true;
				}
				if ( idOn == 135 ) {
					MFont  *font;
					BitMap *text;
					int     xoff;

					/* Put in the right credits / mask the old... */;
					clr = win->Map_Color(0x0000, 0x0000, 0x0000);
					win->FillRectangle(xOff+166, yOff+282, 338, 62, clr);
					clr = win->Map_Color(0x0000, 0x0000, 0x0000);
					if ((font = fontserv->New_Font("New York", 18)) == NULL) {
						error("Can't use New York(18) font! -- Exiting.\n");
						exit(255);
					}
					clr = win->Map_Color(0xFFFF, 0xFFFF, 0x5555);
					text = fontserv->Text_to_BitMap("Port to Linux:   ",
															font, STYLE_NORM);
					xoff = text->width;
					DrawText(xOff+178, yOff+298, text, clr);
					fontserv->Free_Text(text);
					clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
					text = fontserv->Text_to_BitMap("Sam Lantinga",
															font, STYLE_NORM);
					DrawText(xOff+178+xoff, yOff+298, text, clr);
					fontserv->Free_Text(text);
				}
				win->Fade(FADE_STEPS);
				drawscreen = false;
			}
		}

	}
	win->Fade(FADE_STEPS);
	gUpdateBuffer = true;
	gFadeBack = true;
}	/* -- DoAbout */
