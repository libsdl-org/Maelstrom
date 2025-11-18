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

/* Simple array class for use with Maelstrom.
 *
 * The types in the array are POD types, without constructors or destructors.
 */
#ifndef _array_h
#define _array_h

#include <stdlib.h>
#include <assert.h>

template <typename T>
class array
{
public:
	array() : m_len(0), m_max(1), m_data((T*)malloc(sizeof(T))) { }
	~array() { free(m_data); }

	bool find(const T& item) const {
		for (unsigned i = 0; i < m_len; ++i) {
			if (m_data[i] == item) {
				return true;
			}
		}
		return false;
	}
	void clear() {
		m_len = 0;
	}
	void add(const T& item) {
		resize(m_len+1);
		m_data[m_len++] = item;
	}
	void insert(const T& item, unsigned index) {
		if (index == m_len) {
			add(item);
			return;
		}
		resize(m_len+1);
		for (unsigned i = m_len; i > index; --i) {
			m_data[i] = m_data[i-1];
		}
		m_data[index] = item;
		++m_len;
	}
	bool remove(const T& item) {
		for (unsigned i = 0; i < m_len; ++i) {
			if (m_data[i] == item) {
				memmove(&m_data[i], &m_data[i+1], (m_len-i-1)*sizeof(T));
				--m_len;
				return true;
			}
		}
		return false;
	}
	bool empty() const {
		return length() == 0;
	}
	unsigned int length() const {
		return m_len;
	}
	const T& operator[](unsigned index) const {
		assert(index < m_len);
		return m_data[index];
	}
	T& operator[](unsigned index) {
		assert(index < m_len);
		return m_data[index];
	}

protected:
	unsigned m_len;
	unsigned m_max;
	T *m_data;

	void resize(unsigned len) {
		if (len > m_max) {
			while (m_max < len)
				m_max *= 2;
			m_data = (T*)realloc(m_data, m_max*sizeof(T));
		}
	}
};

#endif // _array_h
