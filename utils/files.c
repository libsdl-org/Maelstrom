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

#include "physfs.h"
#include "../external/physfs/extras/physfssdl3.h"

#include "files.h"

#ifndef PATH_MAX
#define PATH_MAX	256
#endif

static const char *storage_org;
static const char *storage_app;
static char datapath[PATH_MAX];
static char modpath[PATH_MAX];
static char modfile[PATH_MAX];

#ifdef MAELSTROM_USE_XDG_DIRS

static bool GetXDGDataPath(const char *directory, char *path, size_t pathlen)
{
	SDL_PathInfo info;
	const char *env = SDL_getenv("XDG_DATA_HOME");
	if (env && *env) {
		SDL_snprintf(path, pathlen, "%s/maelstrom/%s", env, directory);
		if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
			return true;
		}
	} else {
		const char *home = SDL_getenv("HOME");
		if (home && *home) {
			SDL_snprintf(path, pathlen, "%s/.local/share/maelstrom/%s", home, directory);
			if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
				return true;
			}
		}
	}

	bool result = false;
	env = SDL_getenv("XDG_DATA_DIRS");
	if (!env || !*env) {
		env = "/usr/local/share/:/usr/share/";
	}
	char *paths = SDL_strdup(env);
	if (paths) {
		char *saveptr;
		for (char *candidate = SDL_strtok_r(paths, ":", &saveptr); candidate; candidate = SDL_strtok_r(NULL, ":", &saveptr)) {
			SDL_snprintf(path, pathlen, "%s/maelstrom/%s", candidate, directory);
			if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
				result = true;
				break;
			}
		}
		SDL_free(paths);
	}
	return result;
}

#endif // MAELSTROM_USE_XDG_DIRS

static bool InitDataPath(void)
{
	const char *env = SDL_getenv("MAELSTROM_DATA");

	if (env) {
		SDL_strlcpy(datapath, env, sizeof(datapath));
		return true;
	}

#ifdef MAELSTROM_USE_XDG_DIRS
	if (GetXDGDataPath("Data", datapath, sizeof(datapath))) {
		return true;
	}
#endif

#ifdef MAELSTROM_DATA
	SDL_strlcpy(datapath, MAELSTROM_DATA, sizeof(datapath));
	return true;

#else
	const char *basepath = SDL_GetBasePath();
	if (basepath) {
		SDL_snprintf(datapath, sizeof(datapath), "%sData", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}

		SDL_snprintf(datapath, sizeof(datapath), "%s../Data", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}

		SDL_snprintf(datapath, sizeof(datapath), "%s../../Data", basepath);
		if (SDL_GetPathInfo(datapath, NULL)) {
			return true;
		}
	}

	SDL_strlcpy(datapath, ".", sizeof(datapath));
	return true;

#endif // MAELSTROM_DATA
}

static bool InitModPath(void)
{
	const char *env = SDL_getenv("MAELSTROM_MODS");

	if (env) {
		SDL_strlcpy(modpath, env, sizeof(modpath));
		return true;
	}

#ifdef MAELSTROM_USE_XDG_DIRS
	if (GetXDGDataPath("mods", modpath, sizeof(modpath))) {
		return true;
	}
#endif

#ifdef MAELSTROM_MODS
	SDL_strlcpy(modpath, MAELSTROM_MODS, sizeof(modpath));
	return true;

#else
	SDL_strlcpy(modpath, datapath, sizeof(modpath));

	// Trim "Data/"
	{
		size_t len = SDL_strlen(modpath);
		if (len >= 5 && SDL_strcmp(&modpath[len - 5], "Data/") == 0) {
			modpath[len - 5] = '\0';
		}
	}
	SDL_strlcat(modpath, "mods", sizeof(modpath));
	return true;

#endif // MAELSTROM_DATA
}

bool InitFilesystem(const char *argv0, const char *org, const char *app)
{
#ifdef SDL_PLATFORM_ANDROID
	// PhysFS expects PHYSFS_AndroidInit instead of the real arg0
	PHYSFS_AndroidInit init = {
		SDL_GetAndroidJNIEnv(),
		SDL_GetAndroidActivity()
	};
	argv0 = (const char *)&init;
#endif

	storage_org = org;
	storage_app = app;

	if (!InitDataPath()) {
		return false;
	}

	// Make sure datapath ends in '/'
	if (datapath[SDL_strlen(datapath) - 1] != '/') {
		SDL_strlcat(datapath, "/", sizeof(datapath));
	}

	if (!InitModPath()) {
		return false;
	}

	// Make sure modpath ends in '/'
	if (modpath[SDL_strlen(modpath) - 1] != '/') {
		SDL_strlcat(modpath, "/", sizeof(modpath));
	}

	if (!PHYSFS_init(argv0)) {
		SDL_SetError("Couldn't initialize PhysicsFS: %d", PHYSFS_getLastErrorCode());
		return false;
	}

	return true;
}

void QuitFilesystem(void)
{
	PHYSFS_deinit();
}

const char *GetModPath(void)
{
	return modpath;
}

bool SetModFile(const char *file)
{
	if (*modfile) {
		PHYSFS_unmount(modfile);
		*modfile = '\0';
	}

	if (*file) {
		size_t size = 0;
		void *data = SDL_LoadFile(file, &size);
		if (!data) {
			return false;
		}
		if (data) {
			if (!PHYSFS_mountMemory(data, size, SDL_free, file, "/", true)) {
				SDL_SetError("Couldn't mount %s", file);
				SDL_free(data);
				return false;
			}
		}
		SDL_strlcpy(modfile, file, sizeof(modfile));
	}
	return true;
}

SDL_IOStream *OpenRead(const char *file)
{
	SDL_IOStream *stream = PHYSFSSDL3_openRead(file);
	if (!stream) {
		char path[PATH_MAX];
		SDL_snprintf(path, sizeof(path), "%s%s", datapath, file);
		stream = SDL_IOFromFile(path, "rb");
	}
	return stream;
}

char *LoadFile(const char *file)
{
	SDL_IOStream *stream = OpenRead(file);
	if (!stream) {
		return NULL;
	}

	return (char *)SDL_LoadFile_IO(stream, NULL, true);
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
