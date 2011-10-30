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
#include <strings.h>
#include <physfs.h>

#include "UITemplates.h"


UITemplates::UITemplates()
{
	m_data = NULL;
	m_hashTable = NULL;
}

UITemplates::~UITemplates()
{
	if (m_hashTable) {
		hash_destroy(m_hashTable);
	}
	if (m_data) {
		delete[] m_data;
	}
}

bool
UITemplates::Load(const char *file)
{
	PHYSFS_File *fp;
	PHYSFS_sint64 size;
	rapidxml::xml_node<> *node;
	rapidxml::xml_attribute<> *attr;

	fp = PHYSFS_openRead(file);
	if (!fp) {
		fprintf(stderr, "Warning: Couldn't open %s: %s\n",
					file, PHYSFS_getLastError());
		return false;
	}

	size = PHYSFS_fileLength(fp);
	m_data = new char[size+1];
	if (PHYSFS_readBytes(fp, m_data, size) != size) {
		fprintf(stderr, "Warning: Couldn't read from %s: %s\n",
					file, PHYSFS_getLastError());
		PHYSFS_close(fp);
		delete[] m_data;
		m_data = NULL;
		return false;
	}
	m_data[size] = '\0';
	PHYSFS_close(fp);

	try {
		m_doc.parse<0>(m_data);
	} catch (rapidxml::parse_error e) {
		fprintf(stderr, "Warning: Couldn't parse %s: error: %s\n",
					file, e.what());
		delete[] m_data;
		m_data = NULL;
		return false;
	}


	m_hashTable = hash_create(NULL, HashTable_Hash,
					HashTable_KeyMatch,
					HashTable_Nuke);

	node = m_doc.first_node();
	for (node = node->first_node(); node; node = node->next_sibling()) {
		attr = node->first_attribute("templateName", 0, false);
		if (attr) {
			HashKey *key = new HashKey;
			key->type = node->name();
			key->name = attr->value();
			hash_insert(m_hashTable, key, node);
		} else {
			fprintf(stderr, "Warning: UITemplate %s missing 'templateName'\n", node->name());
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
	HashKey key;
	rapidxml::xml_node<> *templateNode;

	if (!m_hashTable) {
		return NULL;
	}

	key.type = type;
	key.name = name;
	if (hash_find(m_hashTable, &key, (const void **)&templateNode)) {
		return templateNode;
	}
	return NULL;
}

// this is djb's xor hashing function.
unsigned
UITemplates::HashTable_Hash(const void *_key, void *data)
{
	const HashKey *key = static_cast<const HashKey *>(_key);
	const char *p;
	register unsigned hash = 5381;

	p = key->type;
	while (*p) {
		hash = ((hash << 5) + hash) ^ *(p++);
	}
	p = key->name;
	while (*p) {
		hash = ((hash << 5) + hash) ^ *(p++);
	}
	return hash;
}

int
UITemplates::HashTable_KeyMatch(const void *_a, const void *_b, void *data)
{
	const HashKey *a = static_cast<const HashKey *>(_a);
	const HashKey *b = static_cast<const HashKey *>(_a);

	return strcasecmp(a->type, b->type) == 0 && 
	       strcasecmp(a->name, b->name) == 0;
}

void
UITemplates::HashTable_Nuke(const void *_key, const void *value, void *data)
{
	HashKey *key = (HashKey *)_key;
	delete key;
}
