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

#ifndef _UITemplates_h
#define _UITemplates_h

#include "../utils/array.h"
#include "../utils/hashtable.h"
#include "../utils/rapidxml.h"

class UITemplates
{
public:
	UITemplates();
	~UITemplates();

	bool Load(const char *file);

	rapidxml::xml_node<> *GetTemplateFor(rapidxml::xml_node<> *node) const;
	rapidxml::xml_node<> *GetTemplate(const char *type, const char *name) const;

protected:
	array<char*> m_data;
	rapidxml::xml_document<> m_doc;
	HashTable *m_hashTable;

protected:
	static unsigned HashTable_Hash(const void *key, void *data);
	static int HashTable_KeyMatch(const void *a, const void *b, void *data);
	static void HashTable_Nuke(const void *key, const void *value, void *data);
};

#endif // _UITemplates_h
