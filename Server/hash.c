#include "includes/chess.h"

int seed = 0;

int get_seed() {
    if (seed == 0) {
        seed = (int) time(NULL);
    }
    seed++;
    return seed;
}

Hash rnd64() {

    const Hash z = 0x9FB21C651E98DF25;

    Hash n = get_seed();

    n ^= ((n << 49) | (n >> 15)) ^ ((n << 24) | (n >> 40));
    n *= z;
    n ^= n >> 35;
    n *= z;
    n ^= n >> 28;

    return n;
}

Hashtable *create_hashtable(int size) {
    Hashtable *table = malloc(sizeof(Hashtable));
    table->table = calloc(size, sizeof(HashEntry *));
    table->size = size;
    return table;
}

void delete_hashtable(Hashtable *table) {
    hash_clear(table);
    free(table->table);
    free(table);
}

void hash_clear(Hashtable *table) {
    if (!table || !table->table) return;
    for (int i = 0; i < table->size; i++) {
        HashEntry *entry = table->table[i];
        while (entry) {
            HashEntry *temp = entry;
            entry = entry->next;
            free(temp->data);
            free(temp);
        }
        table->table[i] = NULL;
    }
}

void hash_insert(Hashtable *table, Hash hash, void *data) {
    int table_index = hash % table->size;

    HashEntry *entry = malloc(sizeof(HashEntry));
    entry->hash = hash;
    entry->data = data;
    entry->next = NULL;

    if (!table->table[table_index]) {
        table->table[table_index] = entry;
    } else {
        entry->next = table->table[table_index];
        table->table[table_index] = entry;
    }
}

int hash_remove(Hashtable *table, Hash hash) {
    int table_index = hash % table->size;
    HashEntry *curr = table->table[table_index];
    HashEntry *prev = NULL;

    while (curr) {
        if (curr->hash == hash) {
            if (!prev) {
                table->table[table_index] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr->data);
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return 1;
}

void *hash_find(Hashtable *table, Hash hash) {
    HashEntry *curr = table->table[hash % table->size];
    while (curr) {
        if (curr->hash == hash) {
            return curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}