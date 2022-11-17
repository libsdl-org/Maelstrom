
#define USE_XBM	  /* Using the pixmap doesn't give us an outlined cursor */
#ifdef USE_XBM
#include "cursor.xbm"
#include "cursorm.xbm"
#else
/* XPM */
static char *cursor_xpm[] = {
/* width height num_colors chars_per_pixel */
"    10    16        3            1",
/* colors */
" 	s None	c None",
"@	c #000000",
".	c #FFFFFF",
/* pixels */
" .        ",
".@.       ",
".@@.      ",
".@@@.     ",
".@@@@.    ",
".@@@@@.   ",
".@@@@@@.  ",
".@@@@@@@. ",
".@@@@@@@@.",
".@@@@@... ",
".@@.@@.   ",
".@. .@@.  ",
" .  .@@.  ",
"     .@@. ",
"     .@@. ",
"      ..  ",
};
#endif

static char * invis_cursor_xpm[] = {
/* width height ncolors chars_per_pixel */
"4 4 1 1",
/* colors */
" 	s None	c None",
/* pixels */
"    ",
"    ",
"    ",
"    "};

