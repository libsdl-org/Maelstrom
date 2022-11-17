
#ifndef _load_h
#define _load_h

#include <stdlib.h>
#include <string.h>
#ifdef WIN32  // For access() prototype
#include <io.h>
#define F_OK	0
#define access	_access
#else
#include <unistd.h>
#endif

#include "SDL_FrameBuf.h"

/* Get the library directory */
#ifdef TESTING
#ifndef LIBDIR
#define LIBDIR	"."
#endif
#else
#ifndef LIBDIR
#ifdef WIN32
#define LIBDIR "."
#else
#define LIBDIR	"/usr/local/lib/Maelstrom"
#endif /* Win32 */
#endif
#endif /* TESTING */

class LibPath {

private:
	static char *exepath;

public:
	static void SetExePath(const char *exe) {
		char *exep;

		exepath = strdup(exe);
		for ( exep = exepath+strlen(exe); exep > exepath; --exep ) {
			if ( (*exep == '/') || (*exep == '\\') ) {
				break;
			}
		}
		if ( exep > exepath ) {
			*exep = '\0';
		} else {
			strcpy(exepath, ".");
		}
	}

public:
	LibPath() {
		path = NULL;
	}
	LibPath(char *file) {
		path = NULL;
		Path(file);
	}
	~LibPath() {
		if ( path ) delete[] path;
	}

	const char *Path(const char *filename) {
		char *directory;

		directory = getenv("MAELSTROM_LIB");
		if ( directory == NULL ) {
			directory = LIBDIR;
			if ( access(directory, F_OK) < 0 ) {
				directory = exepath;
			}
		}

		if ( path != NULL )
			delete[] path;
		path = new char[strlen(directory)+1+strlen(filename)+1];
		sprintf(path, "%s/%s", directory, filename);
		return(path);
	}
	const char *Path(void) {
		return(path);
	}

private:
	char *path;
};

/* Functions exported from load.cc */
extern SDL_Surface *Load_Icon(char **xpm);
extern SDL_Surface *Load_Title(FrameBuf *screen, int title_id);
extern SDL_Surface *GetCIcon(FrameBuf *screen, short cicn_id);

#endif /* _load_h */
