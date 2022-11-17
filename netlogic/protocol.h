
/* Network protocol for synchronization and keystrokes */

#define SYNC_MSG	0x00			/* Sent during game */
#define NEW_GAME	0x01			/* Sent by players at start */
#define NET_ABORT	0x04			/* Used with address server */
#define KEY_PRESS	0x80			/* Sent during game */
#define KEY_RELEASE	0xF0			/* Sent during game */

/* * * * * * * *
	This stuff is shared between netplay.cc and netplayd
*/
/* The default port for Maelstrom games.  What is 0xAEAE?? *shrug* :) */
#define NETPLAY_PORT	0xAEAE			/* port 44718 */

/* The minimum length of a new packet buffer */
#define NEW_PACKETLEN	(3+3*4)

/* Note: if you change MAX_PLAYERS, you need to modify the gPlayerColors
   array in player.cc
*/
#define MAX_PLAYERS		3		/* No more than 255!! */

