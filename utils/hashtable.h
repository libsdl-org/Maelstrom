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

#ifndef _hashtable_h
#define _hashtable_h

#ifdef __cplusplus
extern "C" {
#endif

// Hashtables...

typedef struct HashTable HashTable;
typedef unsigned (*HashTable_HashFn)(const void *key, void *data);
typedef int (*HashTable_KeyMatchFn)(const void *a, const void *b, void *data);
typedef void (*HashTable_NukeFn)(const void *key, const void *value, void *data);

HashTable *hash_create(void *data, const HashTable_HashFn hashfn,
                       const HashTable_KeyMatchFn keymatchfn,
                       const HashTable_NukeFn nukefn);
void hash_destroy(HashTable *table);
int hash_insert(HashTable *table, const void *key, const void *value);
int hash_remove(HashTable *table, const void *key);
int hash_find(const HashTable *table, const void *key, const void **_value);
int hash_iter(const HashTable *table, const void **key, const void **value, void **iter);

unsigned hash_hash_string(const void *sym, void *unused);
int hash_keymatch_string(const void *a, const void *b, void *unused);

#ifdef __cplusplus
}
#endif

#endif // _hashtable_h
