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

#ifndef _files_h
#define _files_h

/* Provide file routines that use PHYSFS on most platforms and SDL on Android */

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

bool InitFilesystem(const char *org, const char *app);
bool FilesystemReady(bool *failed);

SDL_IOStream *OpenRead(const char *file);

/* Returns the contents of the file, or NULL on error.
   You should call SDL_free() on the returned data when you are done with it.
*/
char *LoadFile(const char *file);

SDL_Storage *OpenUserStorage(void);
SDL_IOStream *OpenUserFile(const char *file);
char *LoadUserFile(const char *file);
bool SaveUserFile(const char *file, SDL_IOStream *src);

#ifdef __cplusplus
}
#endif
#endif /* _files_h */
