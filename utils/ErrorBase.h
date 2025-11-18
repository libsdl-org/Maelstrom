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

#ifndef _ErrorBase_h
#define _ErrorBase_h

#include <string.h>
#include <stdarg.h>

#define ERRORBASE_ERRSIZ	1024

class ErrorBase {

public:
	ErrorBase() : errstr(NULL) { }
	virtual ~ErrorBase() {
		ClearError();
	}

	/* Error message routine */
	char *Error(void) {
		return(errstr);
	}

	void ClearError() {
		if (errstr) {
			delete[] errstr;
		}
		errstr = NULL;
	}

protected:
	/* Error message */
	void SetError(const char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		if (!errstr) {
			errstr = new char[ERRORBASE_ERRSIZ];
		}
		SDL_vsnprintf(errstr, ERRORBASE_ERRSIZ, fmt, ap);
		va_end(ap);
        }
	char *errstr;
};

#endif /* _ErrorBase_h */
