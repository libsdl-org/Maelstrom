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

#include "SDL_FrameBuf.h"

#include "UIElementThumbstick.h"

#define DEGREES_TO_RADS(X)	(float)(((X) * 2*M_PI) / 360)

UIElementType UIElementThumbstick::s_elementType;


UIElementThumbstick::UIElementThumbstick(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElement(parent, name, drawEngine)
{
	m_followTouch = false;
	m_sensitiveRadius = 0.0f;
	m_startX = 0;
	m_startY = 0;
	m_finger = 0;
}

UIElementThumbstick::~UIElementThumbstick()
{
	for (unsigned int i = 0; i < m_actions.length(); ++i) {
		if (m_actions[i].action_enter) {
			SDL_free(m_actions[i].action_enter);
		}
		if (m_actions[i].action_leave) {
			SDL_free(m_actions[i].action_leave);
		}
	}
}

bool
UIElementThumbstick::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_node<> *child;
	ThumbstickAction action;

	if (!UIElement::Load(node, templates)) {
		return false;
	}

	LoadBool(node, "followTouch", m_followTouch);
	LoadNumber(node, "sensitiveRadius", m_sensitiveRadius);

	for (child = node->first_node("action", 0, false); child;
	     child = child->next_sibling("action", 0, false)) {
		SDL_zero(action);

		int angle = 0, arc = 0;
		LoadNumber(child, "angle", angle);
		LoadNumber(child, "arc", arc);
		action.arc_begin = DEGREES_TO_RADS(angle - arc/2);
		if (action.arc_begin < 0) {
			action.arc_begin += (float)(2 * M_PI);
		}
		action.arc_end = DEGREES_TO_RADS(angle + arc/2);
		if (action.arc_end > (2 * M_PI)) {
			action.arc_end -= (float)(2 * M_PI);
		}

		LoadNumber(child, "active_radius", action.active_radius);
		LoadString(child, "action_enter", action.action_enter);
		LoadString(child, "action_leave", action.action_leave);

		m_actions.add(action);
	}
	return true;
}

bool
UIElementThumbstick::HandleEvent(const SDL_Event &event)
{
	// Convert the touch coordinate into something useful here
	int x, y;
	float angle, distance;

	switch (event.type) {
	case SDL_FINGERDOWN:
		if (!m_finger) {
			m_startX = CenterX();
			m_startY = CenterY();

			if (!GetTouchPosition(event, x, y) ||
			    !GetTouchAngleAndDistance(x, y, angle, distance)) {
				return false;
			}
			if (m_sensitiveRadius) {
				if (distance > m_sensitiveRadius) {
					return false;
				}
			} else {
				if (!ContainsPoint(x, y)) {
					return false;
				}
			}

			// We're going to accept this event now
			m_finger = event.tfinger.fingerId;

			if (m_followTouch) {
				MoveCenter(x, y);
				m_startX = x;
				m_startY = y;
				angle = 0.0f;
				distance = 0.0f;
			}
			HandleFinger(angle, distance);
			return true;
		}
		break;
	case SDL_FINGERMOTION:
		if (event.tfinger.fingerId == m_finger) {
			if (!GetTouchPosition(event, x, y) ||
			    !GetTouchAngleAndDistance(x, y, angle, distance)) {
				return false;
			}
			if (m_followTouch) {
				MoveCenter(x, y);
			}
			HandleFinger(angle, distance);
			return true;
		}
		break;
	case SDL_FINGERUP:
		if (event.tfinger.fingerId == m_finger) {
			if (m_followTouch &&
			    GetTouchPosition(event, x, y)) {
				MoveCenter(x, y);
			}
			for (unsigned int i = 0; i < m_actions.length(); ++i) {
				SetActionActive(i, false);
			}
			m_finger = 0;
			return true;
		}
		break;
	}
	return false;
}

void
UIElementThumbstick::MoveCenter(int x, int y)
{
	SetPosition(x - Width()/2, y - Height()/2);
}

void
UIElementThumbstick::HandleFinger(float angle, float distance)
{
	for (unsigned int i = 0; i < m_actions.length(); ++i) {
		SetActionActive(i, ShouldActivateAction(i, angle, distance));
	}
}

bool
UIElementThumbstick::GetTouchPosition(const SDL_Event &event, int &x, int &y)
{
	return m_screen->ConvertTouchCoordinates(event.tfinger, &x, &y);
}

bool
UIElementThumbstick::GetTouchAngleAndDistance(int x, int y, float &angle, float &distance)
{
	float a = (float)(x - m_startX);
	float b = (float)(y - m_startY);

	// The angle is in the 0 - 2PI range with 0 being the +Y axis
	angle = (float)(M_PI - SDL_atan2(a, b));
	distance = (float)SDL_sqrt((a*a) + (b*b));
	return true;
}

bool
UIElementThumbstick::ShouldActivateAction(int index, float angle, float distance)
{
	ThumbstickAction *state = &m_actions[index];

	if (distance < state->active_radius) {
		return false;
	}

	if (state->arc_begin != state->arc_end) {
		if (state->arc_begin < state->arc_end) {
			/* Contiguous arc */
			if (angle < state->arc_begin ||
			    angle > state->arc_end) {
				return false;
			}
		} else {
			/* Arc crosses 0 */
			if (angle > state->arc_end &&
			    angle < state->arc_begin) {
				return false;
			}
		}
	}
	return true;
}

void
UIElementThumbstick::SetActionActive(int index, bool active)
{
	ThumbstickAction *state = &m_actions[index];

	if (active == state->active) {
		return;
	}

	state->active = active;

	const char *action;
	if (active) {
		action = state->action_enter;
	} else {
		action = state->action_leave;
	}
	if (action) {
		Action(this, action);
	}
}
