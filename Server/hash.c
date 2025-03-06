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


void hash_insert(Hashtable *table, Hash hash, int eval_score, int depth) {
	int table_index = hash % table->size;

	HashEntry *entry = malloc(sizeof(HashEntry));
	entry->hash = hash;
	entry->next = NULL;
	entry->eval_score = eval_score;
	entry->depth = depth;

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

HashEntry * hash_find(Hashtable *table, Hash hash) {
	int table_index = hash % table->size;
    HashEntry *curr = table->table[table_index];
    while (curr) {
        if (curr->hash == hash) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;  // Not found
}


void display_table(Hashtable *table) {
    if (!table) {
        printf("Hash table is NULL.\n");
        return;
    }
    printf("--start--\n");
    for (int i = 0; i < table->size; i++) {
        HashEntry *entry = table->table[i];
        if (entry) {
            printf("Bucket %d: ", i);
            while (entry) {	
                printf("[Hash: %llu] -> ", entry->hash);
                entry = entry->next;
            }
            printf("NULL\n");
        }
    }
    printf("--end--\n");
}


// int main() {
// 	init_zobrist_hash(&zobrist, NULL);
// 	printf("%llu\n",zobrist.white_turn_hash);

// 	Hashtable *t = create_hashtable(9);

// 	display_table(t);

// 	hash_insert(t, 5, 0, 0);
// 	hash_insert(t, 14, 0, 0);
// 	hash_insert(t, 23, 0, 0);
// 	hash_insert(t, 3, 0, 0);

// 	display_table(t);

// 	hash_remove(t,14);

// 	display_table(t);

// 	HashEntry *entry = hash_find(t, 5);
// 	if (entry) {
// 		printf("found: %d, %d, %llu\n",entry->depth,entry->eval_score, entry->hash);
// 	} else {
// 		printf("no found\n");
// 	}



// 	delete_hashtable(t);

// 	return 0;
// }