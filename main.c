#include <stdio.h>
#include <stdlib.h>
#include "hash_table.h"

int main(int argc, char *argv[]) {

    ht_hash_table *ht = ht_new();

    ht_insert(ht, "chien", "dog");

    printf("Key = 'chien', Value = %s\n", ht_search(ht, "chien"));

    ht_del_hash_table(ht);
    return 0;
}
