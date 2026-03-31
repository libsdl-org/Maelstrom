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

#include <limits.h>	// For PATH_MAX

#include "files.h"

#ifndef PATH_MAX
#define PATH_MAX    256
#endif

static const char *storage_org;
static const char *storage_app;
static char datapath[PATH_MAX];
static char override[PATH_MAX];

bool InitDataPath(void)
{
	const char *env = SDL_getenv("MAELSTROM_DATA");

	if (env) {
		SDL_strlcpy(datapath, env, sizeof(datapath));
		return true;
	}

#ifdef MAELSTROM_DATA
	SDL_strlcpy(datapath, MAELSTROM_DATA, sizeof(datapath));
	return true;

#else
	const char *basepath = SDL_GetBasePath();
	if (basepath) {
		SDL_snprintf(datapath, sizeof(datapath), "%sData/", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}

		SDL_snprintf(datapath, sizeof(datapath), "%s../Data/", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}

		SDL_snprintf(datapath, sizeof(datapath), "%s../../Data/", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}
	}

	SDL_strlcpy(datapath, "./", sizeof(datapath));
	return true;

#endif // MAELSTROM_DATA
}

void InitOverridePath(void)
{
	const char *env = SDL_getenv("MAELSTROM_DATA_OVERRIDE");

	if (env) {
		SDL_strlcpy(override, env, sizeof(override));
		return;
	}

#ifdef MAELSTROM_DATA_OVERRIDE
	SDL_strlcpy(override, MAELSTROM_DATA_OVERRIDE, sizeof(override));
#else
	SDL_snprintf(override, sizeof(override), "%s../addon/", datapath);
#endif

	if (!SDL_GetPathInfo(override, NULL)) {
		override[0] = '\0';
	}
}

bool InitFilesystem(const char *org, const char *app)
{
	storage_org = org;
	storage_app = app;

	if (!InitDataPath()) {
		return false;
	}

	// Make sure the datapath ends in '/'
	if (datapath[SDL_strlen(datapath) - 1] != '/') {
		SDL_strlcat(datapath, "/", sizeof(datapath));
	}

	InitOverridePath();

	// Make sure the override ends in '/'
	if (override[SDL_strlen(override) - 1] != '/') {
		SDL_strlcat(override, "/", sizeof(override));
	}

	return true;
}

SDL_IOStream *OpenRead(const char *file)
{
	SDL_IOStream *stream = NULL;
	char path[PATH_MAX];

	if (*override) {
		SDL_snprintf(path, sizeof(path), "%s%s", override, file);
		stream = SDL_IOFromFile(path, "rb");
	}
	if (!stream) {
		SDL_snprintf(path, sizeof(path), "%s%s", datapath, file);
		stream = SDL_IOFromFile(path, "rb");
	}
	return stream;
}

char *LoadFile(const char *file)
{
	char *data = NULL;
	char path[PATH_MAX];

	if (*override) {
		SDL_snprintf(path, sizeof(path), "%s%s", override, file);
		data = (char *)SDL_LoadFile(path, NULL);
	}
	if (!data) {
		SDL_snprintf(path, sizeof(path), "%s%s", datapath, file);
		data = (char *)SDL_LoadFile(path, NULL);
	}
	return data;
}

SDL_Storage *OpenUserStorage(void)
{
	SDL_Storage *storage = SDL_OpenUserStorage(storage_org, storage_app, 0);
	if (storage) {
		while (!SDL_StorageReady(storage)) {
			SDL_Delay(10);
		}
	}
	return storage;
}

SDL_IOStream *OpenUserFile(const char *file)
{
	SDL_Storage *storage = NULL;
	void *data = NULL;
	Uint64 length = 0;
	SDL_IOStream *fp = NULL;

	storage = OpenUserStorage();
	if (!storage) {
		goto done;
	}

	if (!SDL_GetStorageFileSize(storage, file, &length)) {
		goto done;
	}

	data = SDL_malloc(length);
	if (!data) {
		goto done;
	}

	if (!SDL_ReadStorageFile(storage, file, data, length)) {
		goto done;
	}

	fp = SDL_IOFromConstMem(data, length);
	if (!fp) {
		goto done;
	}

	SDL_SetPointerProperty(SDL_GetIOProperties(fp), SDL_PROP_IOSTREAM_MEMORY_FREE_FUNC_POINTER, SDL_free);

done:
	SDL_CloseStorage(storage);
	if (!fp) {
		SDL_free(data);
	}
	return fp;
}

char *LoadUserFile(const char *file)
{
	return (char *)SDL_LoadFile_IO(OpenUserFile(file), NULL, true);
}

bool SaveUserFile(const char *file, SDL_IOStream *src)
{
	SDL_Storage *storage = NULL;
	void *data;
	Uint64 length;
	bool result = false;

	storage = OpenUserStorage();
	if (!storage) {
		goto done;
	}

	/* Make sure all directories are created */
	if (SDL_strchr(file, '/') != NULL) {
		char *path = SDL_strdup(file);
		if (path) {
			char *next = path + 1;
			while ((next = SDL_strchr(next, '/')) != NULL) {
				*next = '\0';
				SDL_CreateStorageDirectory(storage, path);
				*next++ = '/';
			}
			SDL_free(path);
		}
	}

	data = SDL_GetPointerProperty(SDL_GetIOProperties(src), SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
	if (!data) {
		SDL_SetError("Couldn't get dynamic memory pointer");
		goto done;
	}
	length = SDL_GetIOSize(src);

	if (!SDL_WriteStorageFile(storage, file, data, length)) {
		goto done;
	}

	result = true;

done:
	if (!SDL_CloseStorage(storage)) {
		result = false;
	}
	SDL_CloseIO(src);
	return result;
}
