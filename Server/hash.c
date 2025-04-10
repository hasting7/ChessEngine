#include "includes/chess.h"

Hashtable *create_hashtable(int size) {
    Hashtable *table = malloc(sizeof(Hashtable));
    table->table = calloc(size, sizeof(HashEntry *));
    table->size = size;
    return table;
}

void delete_hashtable(Hashtable *table) {
    for (int i = 0; i < table->size; i++) {
        HashEntry *entry = table->table[i];
        while (entry) {
            HashEntry *temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(table->table);
    free(table);
}


void hash_insert(Hashtable *table, Hash hash, struct board_data board_info) {
	int table_index = hash % table->size;

	HashEntry *entry = malloc(sizeof(HashEntry));
	entry->hash = hash;
	entry->next = NULL;
	entry->data = board_info;

	if (!table->table[table_index]) {
		// table entry is empty
		table->table[table_index] = entry;

	} else {
		// table entry is not empty
		HashEntry *first = table->table[table_index];
		table->table[table_index] = entry;
		entry->next = first;
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
            free(curr);
            return 0;  // Successfully removed
        }
        prev = curr;
        curr = curr->next;
    }
    return 1;  // Not found
}

struct board_data * hash_find(Hashtable *table, Hash hash) {
    HashEntry *curr = table->table[hash % table->size];
    while (curr) {
        if (curr->hash == hash) {
            return &curr->data;
        }
        curr = curr->next;
    }
    return NULL;  // Not found
}
