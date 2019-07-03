#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hash_table.h"
#include "prime.h"

static const int HT_INITIAL_BASE_SIZE = 53;
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
    ht_item *i = malloc(sizeof(ht_item));
    if (i == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }
    i->key = strdup(k);
    i->value = strdup(v);
    return i;
}

// delete an item
static void ht_del_item(ht_item *i) {
    free(i->key);
    free(i->value);
    free(i);
}

// create a new hash table at a particular size
static ht_hash_table *ht_new_sized(const int base_size) {
    ht_hash_table *ht = malloc(sizeof(ht_hash_table));
    if (ht == NULL) {
        printf("Memory allocation error\n");
        exit(1);
    }
    ht->size = base_size;

    ht->size = next_prime(ht->size);

    ht->count = 0;
    ht->items = calloc((size_t)ht->size, sizeof(ht_item*));
    if (ht->items == NULL) {
        printf("Memory allocation error\n");
        exit(1);
    }
    return ht;
}

// create a new hash table
ht_hash_table *ht_new() {
    return ht_new_sized(HT_INITIAL_BASE_SIZE);
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
static void ht_resize(ht_hash_table *ht, const int base_size) {
    // we make sure we're not attempting to reduce the size below minimum
    if (base_size < HT_INITIAL_BASE_SIZE) {
        return;
    }

    // we initialize a new hash table at the desired size
    ht_hash_table *new_ht = ht_new_sized(base_size);

    // all non-NULL or deleted items are inserted into the new table
    int i;
    for (i = 0; i < ht->size; i++) {
        ht_item *item = ht->items[i];
        if (item != NULL && item != &HT_DELETED_ITEM) {
            ht_insert(new_ht, item->key, item->value);
        }
    }

    ht->size = new_ht->size;
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

// the following two functions will simplify resizing
static void ht_resize_up(ht_hash_table *ht) {
    const int new_size = ht->size * 2;
    ht_resize(ht, new_size);
}

static void ht_resize_down(ht_hash_table *ht) {
    const int new_size = ht->size / 2;
    ht_resize(ht, new_size);
}

// insert a key:value pair in the hash table
void ht_insert(ht_hash_table *ht, const char *key, const char *value) {
    // we check if we need to resize up
    const int load = ht->count * 100 / ht->size;
    if (load > 70)
        ht_resize_up(ht);

    ht_item *item = ht_new_item(key, value);
    int index = ht_get_hash(item->key, ht->size, 0);
    ht_item *cur_item = ht->items[index];
    int i = 1;
    while (cur_item != NULL) {
        // if the items was previously deleted
        // we delete it and place the new item in its place
        if (cur_item != &HT_DELETED_ITEM) {
            if (strcmp(cur_item->key, key) == 0) {
                ht_del_item(cur_item);
                ht->items[index] = item;
                return;
            }
        }
        index = ht_get_hash(item->key, ht->size, i);
        cur_item = ht->items[index];
        i++;
    }
    ht->items[index] = item;
    ht->count++;
}

// return the value associated with a key, or NULL if key does not exist
char *ht_search(ht_hash_table *ht, const char *key) {
    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                return item->value;
            }
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
        ht_resize_down(ht);

    int index = ht_get_hash(key, ht->size, 0);
    ht_item *item = ht->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != &HT_DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                ht_del_item(item);
                ht->items[index] = &HT_DELETED_ITEM;
                ht->count--;
            }
        }
        index = ht_get_hash(key, ht->size, i);
        item = ht->items[index];
        i++;
    }
}
