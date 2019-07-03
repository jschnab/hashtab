#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xmalloc.h"
#include "hash_table.h"
#include "prime.h"

// parameters for the hashing algorithm
static const int HT_PRIME_1 = 151;
static const int HT_PRIME_2 = 163;

/* Deleting from an open-addressed hash table is complicated because the item
 * we wish to delete may be part of a collision chain. Removing it from the
 * table would break the chain and make finding items in the tail of the chain
 * impossible. Instead of deleting an item, we mark it as deleted.
*/
static ht_item HT_DELETED_ITEM = {NULL, NULL};

// create a new item
static ht_item *ht_new_item(const char *k, const char *v) {
    ht_item *i = xmalloc(sizeof(ht_item));
    i->key = xstrdup(k);
    i->value = xstrdup(v);
    return i;
}

// delete an item
static void ht_del_item(ht_item *i) {
    free(i->key);
    free(i->value);
    free(i);
}

//create a new hash table at a particular size
static ht_hash_table *ht_new_sized(const int size_index) {
    ht_hash_table *ht = xmalloc(sizeof(ht_hash_table));
    ht->size_index = size_index;
    const int base_size = 50 << ht->size_index;
    ht->size = next_prime(base_size);
    ht->count = 0;
    ht->items = xcalloc((size_t)ht->size, sizeof(ht_item*));
    return ht;
}

// create a new hash table
ht_hash_table *ht_new() {
    return ht_new_sized(0);
}

// delete a hash table
void ht_del_hash_table(ht_hash_table *ht) {
    int i;
    for (i = 0; i < ht->size; i++) {
        ht_item *item = ht->items[i];
        if (item != NULL) {
            ht_del_item(item);
        }
    }
    free(ht->items);
    free(ht);
}

// resize the hash table
static void ht_resize(ht_hash_table *ht, const int direction) {
    // we make sure we're not attempting to reduce the size below minimum
    const int new_size_index = ht->size_index + direction;
    // we don't resize down the smallest hash table
    if (new_size_index < 0) {
        return;
    }

    // we initialize a new hash table at the desired size
    ht_hash_table *new_ht = ht_new_sized(new_size_index);

    // all non-NULL or deleted items are inserted into the new table
    int i;
    for (i = 0; i < ht->size; i++) {
        ht_item *item = ht->items[i];
        if (item != NULL && item != &HT_DELETED_ITEM) {
            ht_insert(new_ht, item->key, item->value);
        }
    }

    ht->size_index = new_ht->size_index;
    ht->count = new_ht->count;

    // we swap attributes of our original and resized hash tables
    // first, we give new_ht ht's size and items
    const int tmp_size = ht->size;
    ht->size = new_ht->size;
    new_ht->size = tmp_size;

    ht_item **tmp_items = ht->items;
    ht->items = new_ht->items;
    new_ht->items = tmp_items;

    ht_del_hash_table(new_ht);
}

// return the hash of 's' between 0 and 'm'
static int ht_hash(const char *s, const int a, const int m) {
    long hash = 0;
    const int len_s = strlen(s);
    int i;
    for (i = 0; i < len_s; i++) {
        hash += (long) pow(a, len_s - i + 1) * s[i];
        hash = hash % m;
    }
    return (int) hash;
}

static int ht_get_hash(
        const char *s,
        const int num_buckets,
        const int attempt
) {
    const int hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
    const int hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
    return (hash_a + attempt * (hash_b + 1)) % num_buckets;
}

/* To resize, we check the load on hash tables during 'insert' and 'delete'.
 * If it is above 0.7 we resize up. If it is below 0.1 we resize down.
 * To avoid doing floating point paths, we multiply the count by 100 and check
 * if it is above 70 or below 10.
 */

// insert a key:value pair in the hash table
void ht_insert(ht_hash_table *ht, const char *key, const char *value) {
    // we check if we need to resize up
    const int load = ht->count * 100 / ht->size;
    if (load > 70)
        ht_resize(ht, 1);

    ht_item *item = ht_new_item(key, value);
    int index = ht_get_hash(item->key, ht->size, 0);
    ht_item *cur_item = ht->items[index];
    int i = 1;
    // cycle through filled buckets until we hit an empty of deleted one
    while (cur_item != NULL && cur_item != &HT_DELETED_ITEM) {
        if (strcmp(cur_item->key, key) == 0) {
            ht_del_item(cur_item);
            ht->items[index] = item;
            return;
        }
        index = ht_get_hash(item->key, ht->size, i);
        cur_item = ht->items[index];
        i++;
    }
    // index points to a free bucket
    ht->items[index] = item;
    ht->count++;
}

// return the value associated with a key, or NULL if key does not exist
char *ht_search(ht_hash_table *ht, const char *key) {
    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];
    int i = 1;
    while (item != NULL && item != &HT_DELETED_ITEM) {
        if (strcmp(item->key, key) == 0) {
                return item->value;
        }
        index = ht_get_hash(key, ht->size, i);
        item = ht->items[index];
        i++;
    }
    return NULL;
}


// delete an item from the hash table or do nothing if key does not exist
void ht_delete(ht_hash_table *ht, const char *key) {
    // we check if we need to resize down
    const int load = ht->count * 100 / ht->size;
    if (load < 10)
        ht_resize(ht, -1);

    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];
    int i = 1;
    while (item != NULL && item != &HT_DELETED_ITEM) {
        if (strcmp(item->key, key) == 0) {
            ht_del_item(item);
            ht->items[index] = &HT_DELETED_ITEM;
            ht->count--;
        }
        index = ht_get_hash(key, ht->size, i);
        item = ht->items[index];
        i++;
    }
}
