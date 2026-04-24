/*
  screenlib:  A simple window and UI library based on the SDL library
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

#include <stdio.h>

#ifdef ENABLE_STEAM
#include <steam/steam_api.h>
#endif

#include "../utils/files.h"
#include "SDL_FrameBuf.h"


/* Constructors cannot fail. :-/ */
FrameBuf::FrameBuf() : ErrorBase()
{
}

int
FrameBuf::Init(int width, int height, Uint32 window_flags, const char *title, SDL_Surface *icon)
{
	int window_width = width;
	int window_height = height;

	// Enlarge the window so it's easy to see
	if (!(window_flags & SDL_WINDOW_FULLSCREEN)) {
		const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		if (mode) {
			while ((window_width * 3) < mode->w && (window_height * 3) < mode->h) {
				window_width *= 2;
				window_height *= 2;
			}
		}
	}

	/* Create the window */
	m_window = SDL_CreateWindow(title, window_width, window_height, window_flags);
	if (!m_window) {
		SetError("Couldn't create %dx%d window: %s", 
					width, height, SDL_GetError());
		return(-1);
	}

	/* Set the icon, if any */
	if (icon) {
		SDL_SetWindowIcon(m_window, icon);
	}

	/* Create the renderer */
	m_renderer = SDL_CreateRenderer(m_window, NULL);
	if (!m_renderer) {
		SetError("Couldn't create renderer: %s", SDL_GetError());
		return(-1);
	}
	SDL_SetDefaultTextureScaleMode(m_renderer, SDL_SCALEMODE_PIXELART);

	Clear();

	/* Set the output area */
	SetLogicalSize(width, height);

	m_width = width;
	m_height = height;
	m_clip.x = 0.0f;
	m_clip.y = 0.0f;
	m_clip.w = (float)width;
	m_clip.h = (float)height;

	return(0);
}

FrameBuf::~FrameBuf()
{
	for (unsigned int i = 0; i < m_gamepads.length(); ++i) {
		SDL_CloseGamepad(m_gamepads[i]);
	}
	if (m_cursor) {
		SDL_DestroyCursor(m_cursor);
	}
	if (m_renderer) {
		SDL_DestroyRenderer(m_renderer);
	}
	if (m_window) {
		SDL_DestroyWindow(m_window);
	}
}

void
FrameBuf::SetLogicalSize(int width, int height)
{
	if (width && height) {
		SDL_SetRenderLogicalPresentation(m_renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	} else {
		SDL_SetRenderLogicalPresentation(m_renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
	}
}

void
FrameBuf::ProcessEvent(SDL_Event *event)
{
	SDL_ConvertEventToRenderCoordinates(m_renderer, event);
	ProcessGamepadEvent(event);
}

void
FrameBuf::OpenGamepad(SDL_JoystickID id)
{
	SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
	if (gamepad) {
		m_gamepads.add(gamepad);
	}
}

void
FrameBuf::CloseGamepad(SDL_JoystickID id)
{
	for (unsigned int i = 0; i < m_gamepads.length(); ++i) {
		SDL_Gamepad *gamepad = m_gamepads[i];
		if (SDL_GetGamepadID(gamepad) == id) {
			SDL_CloseGamepad(gamepad);
			m_gamepads.removeAt(i);
			break;
		}
	}
}

void
FrameBuf::ProcessGamepadEvent(SDL_Event *event)
{
	SDL_Gamepad *gamepad = nullptr;

	if (!GamepadMouseEnabled()) {
		return;
	}

	switch (event->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		OpenGamepad(event->gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		CloseGamepad(event->gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
			if (event->gaxis.value > 0) {
				if (!m_gamepadMouseDown) {
					SDL_Event mouse_event;
					SDL_zero(mouse_event);
					mouse_event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
					mouse_event.button.windowID = SDL_GetWindowID(m_window);
					mouse_event.button.button = SDL_BUTTON_LEFT;
					mouse_event.button.down = true;
					SDL_GetMouseState(&mouse_event.button.x, &mouse_event.button.y);
					SDL_PushEvent(&mouse_event);
					m_gamepadMouseDown = true;
				}
			} else {
				if (m_gamepadMouseDown) {
					SDL_Event mouse_event;
					SDL_zero(mouse_event);
					mouse_event.type = SDL_EVENT_MOUSE_BUTTON_UP;
					mouse_event.button.windowID = SDL_GetWindowID(m_window);
					mouse_event.button.button = SDL_BUTTON_LEFT;
					mouse_event.button.down = false;
					SDL_GetMouseState(&mouse_event.button.x, &mouse_event.button.y);
					SDL_PushEvent(&mouse_event);
					m_gamepadMouseDown = false;
				}
			}
		}
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		gamepad = SDL_OpenGamepad(event->gbutton.which);
		if (gamepad) {
			SDL_Keycode key = SDLK_UNKNOWN;
			switch (SDL_GetGamepadButtonLabel(gamepad, (SDL_GamepadButton)event->gbutton.button)) {
			case SDL_GAMEPAD_BUTTON_LABEL_A:
			case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
				key = SDLK_RETURN;
				break;
			case SDL_GAMEPAD_BUTTON_LABEL_B:
			case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
				key = SDLK_ESCAPE;
				break;
			default:
				if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
					key = SDLK_ESCAPE;
				}
				break;
			}
			if (key != SDLK_UNKNOWN) {
				SDL_Event keyboard_event;
				SDL_zero(keyboard_event);
				bool down = (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
				keyboard_event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
				keyboard_event.key.windowID = SDL_GetWindowID(m_window);
				keyboard_event.key.key = key;
				keyboard_event.key.down = down;
				SDL_PushEvent(&keyboard_event);
			}
			SDL_CloseGamepad(gamepad);
		}
		break;
	default:
		break;
	}
}

void
FrameBuf::SetGamepadMouse(bool enabled)
{
	if (enabled) {
		++m_gamepadMouse;
	} else {
		--m_gamepadMouse;
		SDL_assert(m_gamepadMouse >= 0);
	}

	if (!GamepadMouseEnabled()) {
		m_mouseRemainderX = 0.0f;
		m_mouseRemainderY = 0.0f;
	}
}

static float VectorLengthSquared(const SDL_FPoint &point)
{
	return (point.x * point.x + point.y * point.y);
}

void
FrameBuf::UpdateGamepadMouseMovement()
{
	const float MOUSE_SPEED = 16.0f;
	SDL_FPoint velocity = { 0.0f, 0.0f };
	SDL_FPoint mouse;
	SDL_FPoint delta;

	if (!GamepadMouseEnabled()) {
		return;
	}

	// Get the thumbstick with the most deflection
	const int DEADZONE = 8000;
	float max_length = 0.0f;
	for (unsigned int i = 0; i < m_gamepads.length(); ++i) {
		SDL_Gamepad *gamepad = m_gamepads[i];
		SDL_FPoint axis;
		axis.x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
		axis.y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

		float length = VectorLengthSquared(axis);
		if (length > max_length) {
			velocity = axis;
			max_length = length;
		}
	}
	if (max_length <= (DEADZONE * DEADZONE)) {
		return;
	}

	// Normalize the velocity
	float length = SDL_sqrtf(VectorLengthSquared(velocity));
	velocity.x /= length;
	velocity.y /= length;

	// Remove the deadzone and scale to 1.0
	float scale = (length - DEADZONE) / (SDL_JOYSTICK_AXIS_MAX - DEADZONE);
	velocity.x *= scale;
	velocity.y *= scale;

	SDL_GetMouseState(&mouse.x, &mouse.y);
	SDL_RenderCoordinatesFromWindow(m_renderer, mouse.x, mouse.y, &mouse.x, &mouse.y);

	delta.x = (velocity.x * MOUSE_SPEED) + m_mouseRemainderX;
	delta.y = (velocity.y * MOUSE_SPEED) + m_mouseRemainderY;
	m_mouseRemainderX = SDL_modff(delta.x, &delta.x);
	m_mouseRemainderY = SDL_modff(delta.y, &delta.y);

	mouse.x += delta.x;
	mouse.x = SDL_clamp(mouse.x, 0.0f, (float)m_width);
	mouse.y += delta.y;
	mouse.y = SDL_clamp(mouse.y, 0.0f, (float)m_height);
	SDL_RenderCoordinatesToWindow(m_renderer, mouse.x, mouse.y, &mouse.x, &mouse.y);
	SDL_WarpMouseInWindow(m_window, mouse.x, mouse.y);
}

bool
FrameBuf::ConvertTouchCoordinates(const SDL_TouchFingerEvent &finger, int *x, int *y)
{
	// SDL_ConvertEventToRenderCoordinates() converted these into non-normalized values
	*x = (int)finger.x;
	*y = (int)finger.y;
	return true;
}

void
FrameBuf::SetCursor(SDL_Surface *image, int hotX, int hotY)
{
	if (m_cursor) {
		SDL_DestroyCursor(m_cursor);
		m_cursor = nullptr;
	}
	if (m_cursorTexture) {
		SDL_DestroyTexture(m_cursorTexture);
		m_cursorTexture = nullptr;
	}

#if defined(SDL_PLATFORM_APPLE) && !defined(SDL_PLATFORM_MACOS)
	// We need to draw our own cursor
#else
	int window_width, window_height;
	if (SDL_GetWindowSize(m_window, &window_width, &window_height)) {
		float scale = SDL_floorf((float)window_height / m_height);
		SDL_Surface *scaled = SDL_ScaleSurface(image, (int)(image->w * scale), (int)(image->h * scale), SDL_SCALEMODE_NEAREST);
		if (scaled) {
			m_cursor = SDL_CreateColorCursor(scaled, (int)(hotX * scale), (int)(hotY * scale));
			SDL_DestroySurface(scaled);
		}
	}
	if (!m_cursor) {
		m_cursor = SDL_CreateColorCursor(image, hotX, hotY);
	}
#endif
	if (m_cursor) {
		SDL_SetCursor(m_cursor);
	} else {
		m_cursorTexture = SDL_CreateTextureFromSurface(m_renderer, image);
		m_cursorWidth = image->w;
		m_cursorHeight = image->h;
		m_cursorOffsetX = -hotX;
		m_cursorOffsetY = -hotY;
	}
}

void
FrameBuf::GetCursorPosition(int *x, int *y)
{
	float mouse_x, mouse_y;

	SDL_GetMouseState(&mouse_x, &mouse_y);
	SDL_RenderCoordinatesFromWindow(m_renderer, mouse_x, mouse_y, &mouse_x, &mouse_y);

	*x = (int)mouse_x;
	*y = (int)mouse_y;
}

void
FrameBuf::EnableTextInput(int textfieldX, int textfieldY, int textfieldWidth, int textfieldHeight, bool numeric)
{
	SDL_Rect textrect;
	float x = (float)textfieldX;
	float y = (float)textfieldY;

	SDL_RenderCoordinatesToWindow(m_renderer, x, y, &x, &y);
	textrect.x = (int)x;
	textrect.y = (int)y;
	textrect.w = textfieldWidth;
	textrect.h = textfieldHeight;

	int window_width, window_height;
	if (SDL_GetWindowSize(m_window, &window_width, &window_height)) {
		float scale = (float)window_height / m_height;
		textrect.w = (int)(textrect.w * scale);
		textrect.h = (int)(textrect.h * scale);
	}

#ifdef ENABLE_STEAM
	ISteamUtils *pSteamUtils = SteamUtils();
	if (pSteamUtils) {
		// Don't show the gamepad input when streaming to a phone or tablet
		bool bStreamingToPhoneOrTablet = false;
		ISteamRemotePlay *pSteamRemotePlay = SteamRemotePlay();
		for (uint32 i = 0; i < pSteamRemotePlay->GetSessionCount(); ++i) {
			RemotePlaySessionID_t sessionID = pSteamRemotePlay->GetSessionID(i);

			// Skip Remote Play Together sessions
			if (pSteamRemotePlay->BSessionRemotePlayTogether(sessionID)) {
				continue;
			}

			ESteamDeviceFormFactor eFormFactor = pSteamRemotePlay->GetSessionClientFormFactor(sessionID);
			if (eFormFactor == k_ESteamDeviceFormFactorPhone || 
				eFormFactor == k_ESteamDeviceFormFactorTablet) {
				bStreamingToPhoneOrTablet = true;
				break;
			}
		}
		if (!bStreamingToPhoneOrTablet) {
			pSteamUtils->ShowFloatingGamepadTextInput(k_EFloatingGamepadTextInputModeModeSingleLine, textrect.x, textrect.y, textrect.w, textrect.h);
		}
	}
#endif

	SDL_SetTextInputArea(m_window, &textrect, 0);

	SDL_PropertiesID props = SDL_CreateProperties();
#if 0 // Currently showing the numeric keypad on iPad triggers keyboardWillHide(), stopping text input
	if (numeric) {
		SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_TYPE_NUMBER, SDL_TEXTINPUT_TYPE_NUMBER);
	}
#endif
	SDL_SetBooleanProperty(props, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, false);
	SDL_StartTextInputWithProperties(m_window, props);
	SDL_DestroyProperties(props);
}

void
FrameBuf::DisableTextInput()
{
	SDL_StopTextInput(m_window);

#ifdef ENABLE_STEAM
	ISteamUtils *pSteamUtils = SteamUtils();
	if (pSteamUtils) {
		pSteamUtils->DismissFloatingGamepadTextInput();
	}
#endif
}

void
FrameBuf::QueueBlit(SDL_Texture *src,
			int srcx, int srcy, int srcw, int srch,
			int dstx, int dsty, int dstw, int dsth, clipval do_clip,
			float angle)
{
	SDL_FRect srcrect;
	SDL_FRect dstrect;

	srcrect.x = (float)srcx;
	srcrect.y = (float)srcy;
	srcrect.w = (float)srcw;
	srcrect.h = (float)srch;
	dstrect.x = (float)dstx;
	dstrect.y = (float)dsty;
	dstrect.w = (float)dstw;
	dstrect.h = (float)dsth;
	if (do_clip == DOCLIP) {
		float scaleX = srcrect.w / dstrect.w;
		float scaleY = srcrect.h / dstrect.h;

		if (!SDL_GetRectIntersectionFloat(&m_clip, &dstrect, &dstrect)) {
			return;
		}

		/* Adjust the source rectangle to match */
		srcrect.x += ((dstrect.x - dstx) * scaleX);
		srcrect.y += ((dstrect.y - dsty) * scaleY);
		srcrect.w = (dstrect.w * scaleX);
		srcrect.h = (dstrect.h * scaleY);
	}
	if (angle) {
		SDL_RenderTextureRotated(m_renderer, src, &srcrect, &dstrect, angle, NULL, SDL_FLIP_NONE);
	} else {
		SDL_RenderTexture(m_renderer, src, &srcrect, &dstrect);
	}
}

void
FrameBuf::StretchBlit(const SDL_Rect *_dstrect, SDL_Texture *src, const SDL_Rect *_srcrect)
{
	SDL_FRect *srcrect = NULL, cvtsrcrect;
	SDL_FRect *dstrect = NULL, cvtdstrect;

	if (_srcrect) {
		SDL_RectToFRect(_srcrect, &cvtsrcrect);
		srcrect = &cvtsrcrect;
	}

	if (_dstrect) {
		SDL_RectToFRect(_dstrect, &cvtdstrect);
		dstrect = &cvtdstrect;
	}

	SDL_RenderTexture(m_renderer, src, srcrect, dstrect);
}

void
FrameBuf::Update(void)
{
	UpdateGamepadMouseMovement();

	if (Fading() || m_faded) {
		return;
	}

	if (m_cursorTexture && SDL_CursorVisible()) {
		int x, y;
		GetCursorPosition(&x, &y);

		SDL_FRect dstrect;
		dstrect.x = (float)x + m_cursorOffsetX;
		dstrect.y = (float)y + m_cursorOffsetY;
		dstrect.w = (float)m_cursorWidth;
		dstrect.h = (float)m_cursorHeight;
		SDL_RenderTexture(m_renderer, m_cursorTexture, NULL, &dstrect);
	}

	SDL_RenderPresent(m_renderer);
}

void
FrameBuf::Fade(void)
{
	m_fadeStep = 1;

	SDL_Surface *content = SDL_RenderReadPixels(m_renderer, NULL);
	SDL_DestroyTexture(m_fadeTexture);
	m_fadeTexture = SDL_CreateTextureFromSurface(m_renderer, content);
	SDL_DestroySurface(content);
}

void
FrameBuf::FadeStep(void)
{
	const int max = 32;
	int v = m_faded ? m_fadeStep : max - m_fadeStep;
	Uint8 value = (Uint8)(255 * v / max);
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(m_renderer);
	SDL_SetTextureColorMod(m_fadeTexture, value, value, value);
	SDL_RenderTexture(m_renderer, m_fadeTexture, NULL, NULL);
	SDL_RenderPresent(m_renderer);
	SDL_Delay(10);
	++m_fadeStep;

	if (m_fadeStep > max) {
		FadeComplete();
	}
}

void
FrameBuf::FadeComplete(void)
{
	SDL_SetRenderTarget(m_renderer, nullptr);
	SDL_DestroyTexture(m_fadeTexture);
	m_fadeTexture = nullptr;
	m_faded = !m_faded;
}

int
FrameBuf::ScreenDump(const char *prefix, int x, int y, int w, int h)
{
	SDL_Rect rect;
	SDL_Surface *dump;
	char file[1024];
	int retval = -1;

	if (!w) {
		w = Width();
	}
	if (!h) {
		h = Height();
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	dump = SDL_RenderReadPixels(m_renderer, &rect);
	if (!dump) {
		SetError("%s", SDL_GetError());
		return -1;
	}

	/* Get a suitable new filename */
	SDL_Storage *storage = OpenUserStorage();
	if (storage) {
		bool available = false;
		for ( int which = 0; !available; ++which ) {
			SDL_snprintf(file, sizeof(file), "%s%d.png", prefix, which);
			if (!SDL_GetStorageFileSize(storage, file, NULL)) {
				available = true;
			}
		}
		SDL_CloseStorage(storage);
	}

	SDL_IOStream *fp = SDL_IOFromDynamicMem();
	if (SDL_SavePNG_IO(dump, fp, false)) {
		if (SaveUserFile(file, fp)) {
			retval = 0;
		}
	} else {
		/* Couldn't save the screenshot */
		SDL_CloseIO(fp);
	}
	if (retval < 0) {
		SetError("%s", SDL_GetError());
	}
	SDL_DestroySurface(dump);

	return(retval);
}

SDL_Texture *
FrameBuf::LoadImage(const char *file)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	
	texture = NULL;
	surface = SDL_LoadBMP_IO(OpenRead(file), 1);
	if (surface) {
		texture = LoadImage(surface);
		SDL_DestroySurface(surface);
	}
	return texture;
}

SDL_Texture *
FrameBuf::LoadImage(SDL_Surface *surface)
{
	SDL_Texture *texture;

	texture = SDL_CreateTextureFromSurface(m_renderer, surface);
	if (!texture) {
		SetError("%s", SDL_GetError());
		return NULL;
	}
	return(texture);
}

SDL_Texture *
FrameBuf::LoadImage(int w, int h, Uint32 *pixels)
{
	SDL_Texture *texture;

	texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!texture) {
		SetError("%s", SDL_GetError());
		return NULL;
	}

	if (!SDL_UpdateTexture(texture, NULL, pixels, w*sizeof(Uint32))) {
		SetError("%s", SDL_GetError());
		SDL_DestroyTexture(texture);
		return NULL;
	}
	return(texture);
}

void
FrameBuf::FreeImage(SDL_Texture *image)
{
	SDL_DestroyTexture(image);
}

