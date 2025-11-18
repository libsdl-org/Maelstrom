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

#include <stdio.h>
#include "SDL.h"
#include "files.h"
#include "array.h"
#include "hashtable.h"

#include "prefs.h"


static void
hash_nuke_strings(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
	SDL_free((char*)value);
}

static int
sort_keys(const void *a, const void *b)
{
	return SDL_strcasecmp(*(const char **)a, *(const char **)b);
}

Prefs::Prefs(const char *file)
{
	m_file = SDL_strdup(file);
	m_values = hash_create(NULL, hash_hash_string, hash_keymatch_string, hash_nuke_strings);
	m_dirty = false;
}

Prefs::~Prefs()
{
	SDL_free(m_file);
	hash_destroy(m_values);
}

bool
Prefs::Load()
{
	char *data;
	char *key, *value, *next;

	data = LoadFile(m_file);
	if (!data) {
		/* This is fine, we just haven't written them yet */
		return false;
	}

	key = data;
	while (*key) {
		value = SDL_strchr(key, '=');
		if (!value) {
			break;
		}
		*value++ = '\0';

		next = value;
		while (*next && *next != '\r' && *next != '\n') {
			++next;
		}
		if (*next) {
			*next++ = '\0';
		}

		SetString(key, value, false);

		key = next;
		while (*key && (*key == '\r' || *key == '\n')) {
			++key;
		}
	}
	SDL_free(data);

	return true;
}

static __inline__ bool
writeString(SDL_RWops *fp, const char *string)
{
	size_t len = SDL_strlen(string);
	return (SDL_RWwrite(fp, string, 1, len) == len);
}

bool
Prefs::Save()
{
	SDL_RWops *fp;
	const char *key, *value;
	void *iter;

	// Only hit the disk if something actually changed.
	if (!m_dirty) {
		return true;
	}

	fp = OpenWrite(m_file);
	if (!fp) {
		fprintf(stderr, "Warning: Couldn't open %s: %s\n",
					m_file, SDL_GetError());
		return false;
	}

	array<const char *> keys;
	iter = NULL;
	while (hash_iter(m_values, (const void **)&key, (const void **)&value, &iter)) {
		keys.add(key);
	}
	qsort(&keys[0], keys.length(), sizeof(key), sort_keys);
	
	for (unsigned int i = 0; i < keys.length(); ++i) {
		hash_find(m_values, keys[i], (const void **)&value);
		if (!writeString(fp, keys[i]) ||
		    !writeString(fp, "=") ||
		    !writeString(fp, value) ||
		    !writeString(fp, "\r\n")) {
			fprintf(stderr, "Warning: Couldn't write to %s: %s\n",
						m_file, SDL_GetError());
			SDL_RWclose(fp);
			return false;
		}
	}
	SDL_RWclose(fp);

	m_dirty = false;

	return true;
}

void
Prefs::SetString(const char *key, const char *value, bool dirty)
{
	const char *lastValue;

	if (!value) {
		value = "";
	}
	if (hash_find(m_values, key, (const void **)&lastValue)) {
		if (SDL_strcmp(lastValue, value) == 0) {
			return;
		}
		hash_remove(m_values, key);
	}
	hash_insert(m_values, SDL_strdup(key), SDL_strdup(value));

	if (dirty) {
		m_dirty = true;
	}
}

void
Prefs::SetNumber(const char *key, int value)
{
	char buf[32];

	SDL_snprintf(buf, sizeof(buf), "%d", value);
	SetString(key, buf);
}

void
Prefs::SetBool(const char *key, bool value)
{
	if (value) {
		SetString(key, "true");
	} else {
		SetString(key, "false");
	}
}

const char *
Prefs::GetString(const char *key, const char *defaultValue)
{
	const char *value;

	if (hash_find(m_values, key, (const void **)&value)) {
		return value;
	}
	return defaultValue;
}

int
Prefs::GetNumber(const char *key, int defaultValue)
{
	const char *value;

	if (hash_find(m_values, key, (const void **)&value)) {
		return SDL_atoi(value);
	}
	return defaultValue;
}

bool
Prefs::GetBool(const char *key, bool defaultValue)
{
	const char *value;

	if (hash_find(m_values, key, (const void **)&value)) {
		if (*value == '1' || *value == 't' || *value == 'T') {
			return true;
		} else if (*value == '0' || *value == 'f' || *value == 'F') {
			return false;
		}
	}
	return defaultValue;
}
