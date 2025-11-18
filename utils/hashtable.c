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
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

typedef struct HashItem
{
    const void *key;
    const void *value;
    struct HashItem *next;
    struct HashItem *global_next;
    struct HashItem *global_prev;
} HashItem;

struct HashTable
{
    HashItem **table;
    unsigned table_len;
    HashItem *list;
    void *data;
    HashTable_HashFn hash;
    HashTable_KeyMatchFn keymatch;
    HashTable_NukeFn nuke;
};

static unsigned calc_hash(const HashTable *table, const void *key)
{
    return table->hash(key, table->data) & (table->table_len-1);
} // calc_hash

int hash_find(const HashTable *table, const void *key, const void **_value)
{
    HashItem *i;
    void *data = table->data;
    const unsigned hash = calc_hash(table, key);
    HashItem *prev = NULL;
    for (i = table->table[hash]; i != NULL; i = i->next)
    {
        if (table->keymatch(key, i->key, data))
        {
            if (_value != NULL)
                *_value = i->value;

            // Matched! Move to the front of list for faster lookup next time.
            if (prev != NULL)
            {
                prev->next = i->next;
                i->next = table->table[hash];
                table->table[hash] = i;
            } // if

            return 1;
        } // if

        prev = i;
    } // for

    return 0;
} // hash_find

int hash_iter(const HashTable *table, const void **key,
              const void **value, void **iter)
{
    HashItem *i = *iter;
    if (i == NULL)
        i = table->list;
    else
        i = i->global_next;

    if (i != NULL)
    {
        *key = i->key;
        *value = i->value;
        *iter = i;
        return 1;
    }

    *key = NULL;
    *value = NULL;
    *iter = NULL;
    return 0;
} // hash_iter

int hash_insert(HashTable *table, const void *key, const void *value)
{
    HashItem *item = NULL;
    const unsigned hash = calc_hash(table, key);
    if (hash_find(table, key, NULL))
        return 0;

    // !!! FIXME: grow and rehash table if it gets too saturated.
    item = (HashItem *) malloc(sizeof (HashItem));
    if (item == NULL)
        return -1;

    item->key = key;
    item->value = value;
    item->next = table->table[hash];
    item->global_next = table->list;
    if (table->list) {
        table->list->global_prev = item;
    }
    item->global_prev = NULL;
    table->list = item;
    table->table[hash] = item;

    return 1;
} // hash_insert

int hash_remove(HashTable *table, const void *key)
{
    HashItem *item = NULL;
    HashItem *prev = NULL;
    void *data = table->data;
    const unsigned hash = calc_hash(table, key);
    for (item = table->table[hash]; item != NULL; item = item->next)
    {
        if (table->keymatch(key, item->key, data))
        {
            if (prev != NULL)
                prev->next = item->next;
            else
                table->table[hash] = item->next;

            if (item->global_prev)
                item->global_prev->global_next = item->global_next;
            else
                table->list = item->global_next;

            if (item->global_next)
                item->global_next->global_prev = item->global_prev;

            table->nuke(item->key, item->value, data);
            free(item);
            return 1;
        } // if

        prev = item;
    } // for

    return 0;
} // hash_remove

HashTable *hash_create(void *data, const HashTable_HashFn hashfn,
              const HashTable_KeyMatchFn keymatchfn,
              const HashTable_NukeFn nukefn)
{
    const unsigned initial_table_size = 256;
    const unsigned alloc_len = sizeof (HashItem *) * initial_table_size;
    HashTable *table = (HashTable *) malloc(sizeof (HashTable));
    if (table == NULL)
        return NULL;
    memset(table, '\0', sizeof (HashTable));

    table->table = (HashItem **) malloc(alloc_len);
    if (table->table == NULL)
    {
        free(table);
        return NULL;
    } // if

    memset(table->table, '\0', alloc_len);
    table->table_len = initial_table_size;
    table->data = data;
    table->hash = hashfn;
    table->keymatch = keymatchfn;
    table->nuke = nukefn;
    return table;
} // hash_create

void hash_destroy(HashTable *table)
{
    unsigned i;
    void *data = table->data;
    for (i = 0; i < table->table_len; i++)
    {
        HashItem *item = table->table[i];
        while (item != NULL)
        {
            HashItem *next = item->next;
            table->nuke(item->key, item->value, data);
            free(item);
            item = next;
        } // while
    } // for

    free(table->table);
    free(table);
} // hash_destroy


// this is djb's xor hashing function.
static unsigned hash_string_djbxor(const char *str, size_t len)
{
    register unsigned hash = 5381;
    while (len--)
        hash = ((hash << 5) + hash) ^ *(str++);
    return hash;
} // hash_string_djbxor

unsigned hash_hash_string(const void *sym, void *data)
{
    (void) data;
    return hash_string_djbxor((const char*) sym, strlen((const char *) sym));
} // hash_hash_string

int hash_keymatch_string(const void *a, const void *b, void *data)
{
    (void) data;
    return (strcmp((const char *) a, (const char *) b) == 0);
} // hash_keymatch_string
