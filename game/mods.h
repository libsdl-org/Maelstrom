/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _mods_h
#define _mods_h

#include "../screenlib/UIDialog.h"


class UIElementRadioGroup;

struct Mod
{
	~Mod() {
		SDL_free(path);
		SDL_free(name);
		SDL_free(desc);
	}

	Mod &operator=(const Mod &rhs) {
		SDL_free(path);
		if (rhs.path) {
			path = SDL_strdup(rhs.path);
		} else {
			path = nullptr;
		}

		SDL_free(name);
		if (rhs.name) {
			name = SDL_strdup(rhs.name);
		} else {
			name = nullptr;
		}

		SDL_free(desc);
		if (rhs.desc) {
			desc = SDL_strdup(rhs.desc);
		} else {
			desc = nullptr;
		}

		return *this;
	}

	char *path = nullptr;
	char *name = nullptr;
	char *desc = nullptr;
	bool valid = false;
};

class ModsDialogDelegate : public UIDialogDelegate
{
public:
	ModsDialogDelegate(UIPanel *panel) : UIDialogDelegate(panel) { }
	~ModsDialogDelegate() {
		ClearMods();
	}

	virtual bool OnLoad();
	virtual void OnShow();
	virtual void OnHide();

private:
	struct ModBox
	{
		UIElement *box;
		UIElement *label;
		UIElement *name;
		UIElement *desc;
		UIElement *invalid;
	};

private:
	bool LoadModBox(int index, ModBox *box);
	void LoadMods();
	void AddMod(const char *directory, const char *file);
	void ClearMods();

private:
	array<Mod *> m_mods;
	array<ModBox> m_modBoxes;
	UIElementRadioGroup *m_radioGroup;
};

#endif /* _mods_h */
