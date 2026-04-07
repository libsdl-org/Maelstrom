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

#include "Maelstrom_Globals.h"
#include "mods.h"
#include "physfs.h"
#include "../external/physfs/extras/physfssdl3.h"
#include "../utils/files.h"
#include "../screenlib/UIElementRadio.h"
#include "init.h"

bool
ModsDialogDelegate::OnLoad()
{
	m_radioGroup = m_panel->GetElement<UIElementRadioGroup>("modsRadioGroup");
	if (!m_radioGroup) {
		error("Warning: Couldn't find 'modsRadioGroup'\n");
		return false;
	}

	ModBox box;
	for (int i = 1; LoadModBox(i, &box); ++i) {
		m_modBoxes.add(box);
	}
	return true;
}

void
ModsDialogDelegate::OnShow()
{
	const char *current_mod = prefs->GetString(PREFERENCES_MOD_FILE, "");

	// We can't have a mod open while we're enumerating mod archives
	SetModFile("");
	LoadMods();
	SetModFile(current_mod);

	for (unsigned int i = 0; i < m_modBoxes.length(); ++i) {
		UIElementRadioButton *button = m_radioGroup->GetRadioButton(i + 1);

		ModBox *box = &m_modBoxes[i];
		if (i == 0) {
			box->name->Hide();
			box->desc->Hide();
			box->label->SetText("Original art and sounds");
			box->label->Show();
			box->invalid->Hide();
			box->box->Show();

			if (button) {
				button->SetDisabled(false);
				if (SDL_strcmp(current_mod, "") == 0) {
					button->SetChecked(true);
				}
				button->Show();
			}
		} else {
			unsigned int mod_index = (i - 1);
			if (mod_index < m_mods.length()) {
				Mod *mod = m_mods[i - 1];
				if (mod->desc) {
					box->label->Hide();
					box->name->SetText(mod->name);
					box->name->Show();
					box->desc->SetText(mod->desc);
					box->desc->Show();
				} else {
					box->name->Hide();
					box->desc->Hide();
					box->label->SetText(mod->name);
					box->label->Show();
				}
				if (mod->valid) {
					box->invalid->Hide();
				} else {
					box->invalid->Show();
				}
				box->box->Show();

				if (button) {
					if (mod->valid) {
						button->SetDisabled(false);
					} else {
						button->SetDisabled(true);
					}
					if (SDL_strcmp(current_mod, mod->path) == 0) {
						button->SetChecked(true);
					}
					button->Show();
				}
			} else {
				box->box->Hide();

				if (button) {
					button->Hide();
				}
			}
		}
	}
}

void
ModsDialogDelegate::OnHide()
{
	int value = m_radioGroup->GetValue();
	if (value > 0) {
		const char *current_mod = prefs->GetString(PREFERENCES_MOD_FILE, "");
		const char *selected_mod = "";

		if (value > 1) {
			selected_mod = m_mods[value - 2]->path;
		}
		if (SDL_strcmp(selected_mod, current_mod) != 0) {
			prefs->SetString(PREFERENCES_MOD_FILE, selected_mod);
			RestartInitialization();
		}
	}
}

bool
ModsDialogDelegate::LoadModBox(int index, ModBox *box)
{
	SDL_zerop(box);

	char name[128];
	SDL_snprintf(name, sizeof(name), "mod%d", index);
	box->box = m_panel->GetElement<UIElement>(name);
	if (!box->box) {
		return false;
	}

	box->label = box->box->GetElement<UIElement>("label");
	box->name = box->box->GetElement<UIElement>("name");
	box->desc = box->box->GetElement<UIElement>("desc");
	box->invalid = box->box->GetElement<UIElement>("invalid");
	if (!box->label || !box->name || !box->desc || !box->invalid) {
		return false;
	}
	return true;
}

static int SDLCALL CompareMod(const void *a, const void *b)
{
	const Mod *A = *reinterpret_cast<const Mod * const *>(a);
	const Mod *B = *reinterpret_cast<const Mod * const *>(b);
	return SDL_strcasecmp(A->name, B->name);
}

void
ModsDialogDelegate::LoadMods()
{
	const char *modpath = GetModPath();

	ClearMods();

	char **files = SDL_GlobDirectory(modpath, "*.zip", 0, nullptr);
	if (files) {
		for (int i = 0; files[i]; ++i) {
			AddMod(modpath, files[i]);
		}
		SDL_free(files);
	}

	if (m_mods.length() > 0) {
		SDL_qsort(&m_mods[0], m_mods.length(), sizeof(m_mods[0]), CompareMod);
	}
}

static char *GetString(const char *line)
{
	while (SDL_isspace(*line)) {
		++line;
	}

	size_t len = SDL_strlen(line);
	while (len > 0 && SDL_isspace(line[len - 1])) {
		--len;
	}
	if (len > 0) {
		return SDL_strndup(line, len);
	}
	return nullptr;
}

void
ModsDialogDelegate::AddMod(const char *directory, const char *file)
{
	Mod *mod = new Mod;

	if (SDL_asprintf(&mod->path, "%s%s", directory, file) < 0) {
		delete mod;
		return;
	}

	size_t size = 0;
	void *data = SDL_LoadFile(mod->path, &size);
	if (data) {
		if (PHYSFS_mountMemory(data, size, nullptr, mod->path, "/mod", true)) {
			// Look for any of the data directories
			const char *valid_dirs[] = {
				"Fonts",
				"Images",
				"Sounds",
				"Sprites",
				"UI"
			};
			char **rc = PHYSFS_enumerateFiles("/mod");
			for (int i = 0; rc[i] && !mod->valid; ++i) {
				for (int j = 0; j < SDL_arraysize(valid_dirs); ++j) {
					if (SDL_strcmp(rc[i], valid_dirs[j]) == 0) {
						mod->valid = true;
						break;
					}
				}
			}
			PHYSFS_freeList(rc);

			// Load a description file, if any
			SDL_IOStream *stream = PHYSFSSDL3_openRead("/mod/README.txt");
			if (stream) {
				char *text = (char *)SDL_LoadFile_IO(stream, nullptr, true);
				if (text) {
					char *line, *next;
					for ((line = SDL_strtok_r(text, "\n", &next)); line; (line = SDL_strtok_r(NULL, "\n", &next))) {
						if (SDL_strncasecmp(line, "Name:", 5) == 0) {
							mod->name = GetString(line + 5);
						} else if (SDL_strncasecmp(line, "Description:", 12) == 0) {
							mod->desc = GetString(line + 12);
						}
					}
					SDL_free(text);
				}
			}

			PHYSFS_unmount(mod->path);
		}
		SDL_free(data);
	}

	if (!mod->name) {
		mod->name = SDL_strdup(file);
		if (!mod->name) {
			delete mod;
			return;
		}
		mod->name[SDL_strlen(mod->name) - 4] = '\0';
		for (char *spot = mod->name; *spot; ++spot) {
			if (*spot == '_') {
				*spot = ' ';
			}
		}
	}

	m_mods.add(mod);
}

void
ModsDialogDelegate::ClearMods()
{
	for (unsigned int i = 0; i < m_mods.length(); ++i) {
		delete m_mods[i];
	}
	m_mods.clear();
}
