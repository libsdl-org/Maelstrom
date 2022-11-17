
/* This module defines the SoundServer command protocol */

#define NUM_CHANNELS	4
#define CHANNEL_ONE	0
#define CHANNEL_TWO	1
#define CHANNEL_THREE	2
#define CHANNEL_FOUR	3

/* The Client Side */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Load command format:  l<soundnum>				*/
#define LOAD_FORMAT	"%c-%hd"
#define LOAD_CMD	'l'

/* Play command format:  p<soundnum><channel>			*/
#define PLAY_FORMAT	"%c-%hd-%hu"
#define PLAY_CMD	'p'

/* Set volume command:   v<volume>				*/
#define VOL_FORMAT	"%c-%hu"
#define VOL_CMD		'v'

/* Halt play command:    h<channel>				*/
#define HALT_FORMAT	"%c-%hu"
#define HALT_CMD	'h'

/* Halt all play cmd:    H					*/
#define HALTALL_FORMAT	"%c"
#define HALTALL_CMD	'H'

/* Query command format: ?					*/
#define QUERY_FORMAT	"%c"
#define QUERY_CMD	'?'

/* Quit command format:  q					*/
#define QUIT_FORMAT	"%c"
#define QUIT_CMD	'q'


/* The Server Side */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Command response:     Y					*/
#define OKAY_FORMAT	"%c"
#define OKAY_CMD	'Y'

/* Command response:     N					*/
#define NOKAY_FORMAT	"%c"
#define NOKAY_CMD	'N'

/* Length of a command response                                 */
#define REPLY_LEN	2

/* Callback event:       C<channel>				*/
#define CALLBACK_FORMAT	"%c-%hu"
#define CALLBACK_CMD	'C'
