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

#ifndef _packet_h
#define _packet_h

#include "protocol.h"

struct IPaddress {
	IPaddress() { }
	IPaddress(const IPaddress &rhs) {
		*this = rhs;
	}
	~IPaddress() {
		Reset();
	}

	void Reset() {
		if (host) {
			NET_UnrefAddress(host);
			host = nullptr;
		}
		port = 0;
	}

	IPaddress &operator=(const IPaddress &rhs) {
		Reset();
		host = NET_RefAddress(rhs.host);
		port = rhs.port;
		return *this;
	}

    NET_Address *host = nullptr;
	Uint16 port = 0;
};


// Utility functions to compare IP addresses 

extern inline bool operator==(const IPaddress &lhs, const IPaddress &rhs) {
	return lhs.host == rhs.host && lhs.port == rhs.port;
}
extern inline bool operator!=(const IPaddress &lhs, const IPaddress &rhs) {
	return !operator==(lhs, rhs);
}

// A dynamic packet class that takes care of allocating memory and packing data

class DynamicPacket
{
public:
	DynamicPacket() {
	}
	~DynamicPacket() {
		if (maxlen > 0) {
			SDL_free(data);
		}
	}

	void StartLobbyMessage(int msg) {
		Reset();
		Write((Uint8)LOBBY_MSG);
		Write((Uint8)msg);
	}

	void Reset() {
		len = 0;
		pos = 0;
	}

	void Seek(int offset) {
		if (offset < len) {
			pos = offset;
		}
	}

	int Tell() {
		return pos;
	}
	int Size() {
		return len;
	}
	Uint8 *Data() {
		return data;
	}

	void Write(Uint8 value) {
		Grow(sizeof(value));
		data[pos++] = value;
	}
	void Write(Uint16 value) {
		Grow(sizeof(value));
		SDL_memcpy(&data[pos], &value, sizeof(value));
		pos += sizeof(value);
	}
	void Write(Uint32 value) {
		Grow(sizeof(value));
		SDL_memcpy(&data[pos], &value, sizeof(value));
		pos += sizeof(value);
	}
	void Write(const char *value) {
		if (!value) {
			value = "";
		}

		size_t amount = SDL_strlen(value);
		if (amount > 255) {
			amount = 255;
		}
		Write((Uint8)amount);
		Write(value, amount);
	}
	void Write(NET_Address *address) {
		Write(NET_GetAddressString(address));
	}
	void Write(DynamicPacket &packet) {
		size_t amount = packet.len - packet.pos;
		Write(&packet.data[packet.pos], amount);
		packet.pos += (int)amount;
	}
	void Write(const void *value, size_t size) {
		Grow(size);
		SDL_memcpy(&data[pos], value, size);
		pos += (int)size;
	}

	bool Read(Uint8 &value) {
		if (pos+sizeof(value) > (size_t)len) {
			return false;
		}
		value = data[pos++];
		return true;
	}
	bool Read(Uint16 &value) {
		if (pos+sizeof(value) > (size_t)len) {
			return false;
		}
		SDL_memcpy(&value, &data[pos], sizeof(value));
		pos += sizeof(value);
		return true;
	}
	bool Read(Uint32 &value) {
		if (pos+sizeof(value) > (size_t)len) {
			return false;
		}
		SDL_memcpy(&value, &data[pos], sizeof(value));
		pos += sizeof(value);
		return true;
	}
	bool Read(NET_Address *&address) {
		char hostname[MAX_HOSTNAME_LEN];
		if (!Read(hostname, sizeof(hostname))) {
			return false;
		}
		if (hostname[0]) {
			address = NET_ResolveHostname(hostname);
			if (!address) {
				return false;
			}
			if (NET_WaitUntilResolved(address, -1) != NET_SUCCESS) {
				NET_UnrefAddress(address);
				address = nullptr;
				return false;
			}
		}
		return true;
	}
	bool Read(char *value, size_t maxlen) {
		Uint8 amount;
		if (!Read(amount)) {
			return false;
		}
		if (amount > maxlen-1) {
			return false;
		}
		SDL_memcpy(value, &data[pos], amount);
		value[amount] = '\0';
		pos += amount;
		return true;
	}

	void Grow(size_t additionalSize) {
		if (len+additionalSize > (size_t)maxlen) {
			if (maxlen == 0) {
				maxlen = 1024;
			}
			while (len+additionalSize > (size_t)maxlen) {
				maxlen *= 2;
			}
			data = (Uint8*)SDL_realloc(data, maxlen);
		}
		len += (int)additionalSize;
	}

public:
	Uint8 *data = nullptr;
	int len = 0;
	int maxlen = 0;
	IPaddress address;

protected:
	int pos = 0;
};

#endif // _packet_h
