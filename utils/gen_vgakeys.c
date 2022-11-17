
/* Linux compatibility routines */
#include <sys/types.h>
#include <sys/time.h>
#include <linux/kd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "../keyboard.h"

#define UP_BIT	0x80

unsigned long oldmodes;
int kbd_fd;

void kbd_end()
{
   (void) ioctl(kbd_fd, KDSKBMODE, oldmodes);
   system("stty -raw echo");
}

void quit(int sig)
{
	(void) kbd_end();
	printf("Quitting with signal: %d\n", sig);
	exit(sig);
}

void kbd_init()
{
   if ( (kbd_fd=open("/dev/tty", O_RDONLY, 0)) < 0) {
	perror("init_kbd()");
	exit(3);
   }
   if ( ioctl(kbd_fd, KDGKBMODE, &oldmodes) < 0 ) {
	perror("Can't get console keyboard modes");
	exit(3);
   }
   if ( ioctl(kbd_fd, KDSKBMODE, K_RAW) < 0 ) {
	perror("Can't set console keyboard modes");
	exit(3);
   }
   system("stty raw; stty -echo");

   signal(SIGINT, quit);
   signal(SIGQUIT, quit);
}


static KeySym keysyms[256];

main() {
	int i, j;
	unsigned char key;

	for ( i=0; i<256; ++i )
		keysyms[i] = 0x0000;

	kbd_init();

	for ( i=0; keycodes[i].name; ++i ) {
		fprintf(stderr, "Press the '%s' key: ", keycodes[i].name);
	getit:
		read(kbd_fd, &key, 1);
		if ( key & UP_BIT )
			goto getit;
		fprintf(stderr, "%c\r\n", key);
		if ( keysyms[key] ) {
			fprintf(stderr, "'%s' already in keymap!\r\n",
							keycodes[i].name);
		} else
			keysyms[key] = keycodes[i].key;
	}
	kbd_end();

	printf("static KeySym vga_keys[256] = {\n\t");
	for ( i=1; i<=256; ++i ) {
		printf("0x%.4x, ", keysyms[i-1]);
		if ( !(i%8) )
			printf("\n\t");
	}
	printf("};\n");
	fprintf(stderr, "Keymap generation complete.\n");
	exit(0);
}
