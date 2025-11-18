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

#ifndef _prefs_h
#define _prefs_h

struct HashTable;

class Prefs
{
public:
	Prefs(const char *file);
	virtual ~Prefs();

	bool Load();
	bool Save();

	void SetString(const char *key, const char *value, bool dirty = true);
	void SetNumber(const char *key, int value);
	void SetBool(const char *key, bool value);
	void Set(const char *key, const char *value) {
		SetString(key, value);
	}
	void Set(const char *key, int value) {
		SetNumber(key, value);
	}
	void Set(const char *key, bool value) {
		SetBool(key, value);
	}

	const char *GetString(const char *key, const char *defaultValue = 0);
	int GetNumber(const char *key, int defaultValue = 0);
	bool GetBool(const char *key, bool defaultValue = false);
	void Get(const char *key, const char *&value, const char *defaultValue) {
		value = GetString(key, defaultValue);
	}
	void Get(const char *key, int &value, int defaultValue) {
		value = GetNumber(key, defaultValue);
	}
	void Get(const char *key, bool &value, bool defaultValue) {
		value = GetBool(key, defaultValue);
	}

protected:
	char *m_file;
	HashTable *m_values;
	bool m_dirty;
};

template <typename T>
class PrefsVariable
{
public:
	PrefsVariable(const char *name, const T &rhs) {
		m_prefs = 0;
		m_name = name;
		m_defaultValue = m_value = rhs;
	}

	PrefsVariable& operator =(const T &rhs) {
		m_value = rhs;
		if (m_prefs) {
			m_prefs->Set(m_name, m_value);
		}
		return *this;
	}

	PrefsVariable& operator =(const PrefsVariable<T> &rhs) {
		return *this = rhs.m_value;
	}

	operator const T() const {
		return m_value;
	}

	T& operator++() {
		*this = m_value + 1;
		return m_value;
	}
	T operator++(int) {
		T value = m_value;
		*this = m_value + 1;
		return value;
	}
	T& operator--() {
		*this = m_value - 1;
		return m_value;
	}
	T operator--(int) {
		T value = m_value;
		*this = m_value - 1;
		return value;
	}

	void Bind(Prefs *prefs) {
		m_prefs = prefs;
		m_prefs->Get(m_name, m_value, m_defaultValue);
	}

	T Value() {
		return m_value;
	}

	void Reset() {
		*this = m_defaultValue;
	}

protected:
	Prefs *m_prefs;
	const char *m_name;
	T m_value;
	T m_defaultValue;
};

#endif /* _prefs_h */
