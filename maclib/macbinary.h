/*
    MACLIB:  A companion library to SDL for working with Macintosh (tm) data
    Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
