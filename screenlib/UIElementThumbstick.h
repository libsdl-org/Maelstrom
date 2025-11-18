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

#ifndef _UIElementThumbstick_h
#define _UIElementThumbstick_h

#include "UIElement.h"


class UIElementThumbstick : public UIElement
{
DECLARE_TYPESAFE_CLASS(UIElement)
public:
	UIElementThumbstick(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	virtual ~UIElementThumbstick();

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	override bool HandleEvent(const SDL_Event &event);

protected:
	void MoveCenter(int x, int y);
	void HandleFinger(float angle, float distance);
	bool GetTouchPosition(const SDL_Event &event, int &x, int &y);
	bool GetTouchAngleAndDistance(int x, int y, float &angle, float &distance);
	bool ShouldActivateAction(int index, float angle, float distance);
	void SetActionActive(int index, bool active);

protected:
	struct ThumbstickAction {
		bool active;
		float arc_begin;
		float arc_end;
		float active_radius;
		char *action_enter;
		char *action_leave;
	};

	bool m_followTouch;
	float m_sensitiveRadius;
	int m_startX;
	int m_startY;
	SDL_FingerID m_finger;
	array<ThumbstickAction> m_actions;
};

#endif // _UIElementThumbstick_h
