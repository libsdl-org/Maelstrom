/*
  maclib:  A companion library to SDL for working with Macintosh (tm) data
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

/* This is the general format of a MacBinary archive
   This information is taken from /usr/share/magic - thanks! :)
   Verified by looking at:
        http://www.lazerware.com/formats/macbinary/macbinary.html
*/

/* This uses a class because the data doesn't align properly as a struct */

class MBHeader
{
public:
    MBHeader() { }

    Uint16 Version() const {
        Uint16 version = (data[122] << 0) |
                         (data[123] << 8);
        return version;
    }

    Uint32 DataLength() const {
        Uint32 length = (data[83] <<  0) |
                        (data[84] <<  8) |
                        (data[85] << 16) |
                        (data[86] << 24);
        return length;
    }

    Uint32 ResourceLength() const {
        Uint32 length = (data[87] <<  0) |
                        (data[88] <<  8) |
                        (data[89] << 16) |
                        (data[90] << 24);
        return length;
    }

public:
    Uint8 data[128];
};
