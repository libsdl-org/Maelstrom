/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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
#include "../utils/files.h"
#include "../utils/loadxml.h"

#include "UITemplates.h"


UITemplates::UITemplates()
{
	m_hashTable = NULL;
}

UITemplates::~UITemplates()
{
	if (m_hashTable) {
		hash_destroy(m_hashTable);
	}
	for (unsigned int i = 0; i < m_data.length(); ++i) {
		SDL_free(m_data[i]);
	}
}

bool
UITemplates::Load(const char *file)
{
	char *data;
	rapidxml::xml_node<> *node;
	rapidxml::xml_attribute<> *attr;

	data = LoadFile(file);
	if (!data) {
		return false;
	}
	m_data.add(data);

#ifdef RAPIDXML_NO_EXCEPTIONS
	gLoadXMLError = NULL;
	m_doc.parse<0>(data);
	if (gLoadXMLError) {
		return false;
	}
#else
	try {
		m_doc.parse<0>(data);
	} catch (rapidxml::parse_error e) {
		return false;
	}
#endif // RAPIDXML_NO_EXCEPTIONS

	m_hashTable = hash_create(NULL, HashTable_Hash,
					HashTable_KeyMatch,
					HashTable_Nuke);

	node = m_doc.first_node();
	for (node = node->first_node(); node; node = node->next_sibling()) {
		attr = node->first_attribute("templateName", 0, false);
		if (attr) {
			hash_insert(m_hashTable, attr->value(), node);
		} else {
			SDL_Log("Warning: UITemplate %s missing 'templateName'", node->name());
		}
	}
	return true;
}

rapidxml::xml_node<> *
UITemplates::GetTemplateFor(rapidxml::xml_node<> *node) const
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute("template", 0, false);
	if (!attr) {
		return NULL;
	}
	return GetTemplate(node->name(), attr->value());
}

rapidxml::xml_node<> *
UITemplates::GetTemplate(const char *type, const char *name) const
{
	rapidxml::xml_node<> *templateNode;

	if (!m_hashTable) {
		return NULL;
	}

	if (hash_find(m_hashTable, name, (const void **)&templateNode)) {
		return templateNode;
	}
	return NULL;
}

// this is djb's xor hashing function.
unsigned
UITemplates::HashTable_Hash(const void *_key, void *data)
{
	const char *key = static_cast<const char *>(_key);
	register unsigned hash = 5381;

	while (*key) {
		hash = ((hash << 5) + hash) ^ SDL_toupper(*key);
		++key;
	}
	return hash;
}

int
UITemplates::HashTable_KeyMatch(const void *_a, const void *_b, void *data)
{
	const char *a = static_cast<const char *>(_a);
	const char *b = static_cast<const char *>(_b);

	return SDL_strcasecmp(a, b) == 0;
}

void
UITemplates::HashTable_Nuke(const void *_key, const void *value, void *data)
{
}
