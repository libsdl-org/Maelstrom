/*
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

#ifndef _rapidxml_h
#define _rapidxml_h

#ifndef USE_STL
#define RAPIDXML_NO_STDLIB

#include <assert.h>
#include <sys/types.h>

namespace std
{
	typedef ::size_t size_t;
}

#ifdef _MSC_VER
// warning C4291: 'void *operator new(size_t,void *)' : no matching operator delete found; memory will not be freed if initialization throws an exception
#pragma warning(disable:4291)
#endif
extern inline void * operator new (size_t, void * p) throw() { return p ; }

#endif // !USE_STL

#include "rapidxml.hpp"

#endif // _rapidxml_h
