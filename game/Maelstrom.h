/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "rect.h"

#ifndef VERSION
#define VERSION "4.0.0"
#endif
#define	VERSION_STRING		VERSION ".N"

#define GAME_WIDTH		640
#define GAME_HEIGHT		480

#define	SOUND_DELAY		6
#define	FADE_STEPS		40

/* Time in 60'th of second between frames */
#define FRAME_DELAY		2
#define FRAME_DELAY_MS		((FRAME_DELAY*1000)/60)

#define DEFAULT_START_WAVE	1
#define DEFAULT_START_LIVES	3
#define DEFAULT_START_TURBO	0

#define MAX_SPRITES		100
#define MAX_SPRITE_FRAMES	60
#define	MAX_STARS		30
#define	SHIP_FRAMES		48
#define	SPRITES_WIDTH		32
#define	SPRITE_PRECISION	4	/* sprite <--> game precision */
#define	VEL_FACTOR		4
#define	VEL_MAX			(8<<SPRITE_PRECISION)
#define	SCALE_FACTOR		16
#define	SHAKE_FACTOR		256
#define	MIN_BAD_DISTANCE	64

#define NO_PHASE_CHANGE		-1	/* Sprite doesn't change phase */

#define	MAX_SHOTS		18
#define	SHOT_SIZE		4
#define	SHOT_SCALE_FACTOR	4

#define ICON_SIZE		8
#define SHIELD_SIZE		32
#define THRUST_SIZE		16

#define	STATUS_HEIGHT		14
#define	SHIELD_WIDTH		55
#define	INITIAL_BONUS		2000

#define	PLAYER_HITS		3
#define	ENEMY_HITS		3
#define	HOMING_HITS		9
#define	STEEL_SPECIAL		10
#define DEFAULT_HITS		1

#define	NEW_LIFE		50000
#define	SMALL_ROID_PTS		300
#define	MEDIUM_ROID_PTS		100
#define	BIG_ROID_PTS		50
#define	GRAVITY_PTS		500
#define	HOMING_PTS		700
#define	NOVA_PTS		1000
#define	STEEL_PTS		100
#define	ENEMY_PTS		1000
#define	PLAYER_PTS		1000
#define	DEFAULT_PTS		0

#define	HOMING_MOVE		6
#define GRAVITY_MOVE		3

#define	BLUE_MOON		50
#define	MOON_FACTOR		4
#define	NUM_PRIZES		8
#define	LUCK_ODDS		3

#define	INITIAL_SHIELD		((60/FRAME_DELAY) * 3)
#define	SAFE_TIME		(120/FRAME_DELAY)
#define	MAX_SHIELD		((60/FRAME_DELAY) * 5)
#define	DISPLAY_DELAY		(60/FRAME_DELAY)
#define	BONUS_DELAY		(30/FRAME_DELAY)
#define	STAR_DELAY		(30/FRAME_DELAY)
#define	DEAD_DELAY		(3 * (60/FRAME_DELAY))
#define	BOOM_MIN		(20/FRAME_DELAY)
#define	ENEMY_SHOT_DELAY	(10/FRAME_DELAY)

#define	PRIZE_DURATION		(10 * (60/FRAME_DELAY))
#define	MULT_DURATION		(6 * (60/FRAME_DELAY))
#define	BONUS_DURATION		(10 * (60/FRAME_DELAY))
#define	SHOT_DURATION		(1 * (60/FRAME_DELAY))
#define	POINT_DURATION		(2 * (60/FRAME_DELAY))
#define	DAMAGED_DURATION	(10 * (60/FRAME_DELAY))
#define	FREEZE_DURATION		(10 * (60/FRAME_DELAY))
#define	SHAKE_DURATION		(5 * (60/FRAME_DELAY))

/* ----------------------------------------------------------------- */
/* -- Structures and typedefs */

typedef struct {
	int h;
	int v;
} MPoint;

typedef struct {
	int		xCoord;
	int		yCoord;
	unsigned long	color;
} Star, *StarPtr;

/* Sprite blitting information structure */
typedef	struct {
	int numFrames;
	int isSmall;
	Rect hitRect;
	UITexture *sprite[MAX_SPRITE_FRAMES];
	Uint8 *mask[MAX_SPRITE_FRAMES];
} Blit, *BlitPtr;

typedef struct {
	int damage;
	int x;
	int y;
	int xvel;
	int yvel;
	int ttl;
	Rect hitRect;
} Shot, *ShotPtr;

