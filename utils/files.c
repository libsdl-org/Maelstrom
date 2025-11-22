/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "../external/physfs/extras/physfssdl3.h"
#include "files.h"

/* Provide file routines that use PHYSFS on most platforms and SDL on Android */

SDL_IOStream *OpenRead(const char *fname)
{
#ifdef __ANDROID__
	return SDL_IOFromFile(fname, "rb");
#else
	return PHYSFSSDL3_openRead(fname);
#endif
}

SDL_IOStream *OpenWrite(const char *fname)
{
#ifdef __ANDROID__
	return SDL_IOFromFile(fname, "wb");
#else
	return PHYSFSSDL3_openWrite(fname);
#endif
}

char *LoadFile(const char *fname)
{
	SDL_IOStream *fp;
	Sint64 size;
	char *data;

	fp = OpenRead(fname);
	if (!fp) {
		return NULL;
	}

	size = SDL_GetIOSize(fp);
	data = (char*)SDL_malloc(size+1);
	if (SDL_ReadIO(fp, data, size)) {
		data[size] = '\0';
	} else {
		SDL_free(data);
		data = NULL;
	}
	SDL_CloseIO(fp);
	return data;
}
