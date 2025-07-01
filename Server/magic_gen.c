#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

typedef uint64_t Bitboard;
typedef int Hash;
#define HASH_TABLE_SIZE (100003)
#define CHECK_MAGIC_BITBOARDS (100000)

typedef struct hash_entry {
	Hash hash;
	Bitboard move_bitboard;
	struct hash_entry *next;
} HashEntry;

typedef struct hashtable_struct {
	int size;
	struct hash_entry **table;
} Hashtable;

typedef struct magic_tile_response {
	int bits_used;
	Bitboard magic_bitboard;
	Bitboard *array_of_moves;
} MagicData;

struct thread_args {
	int index;
	int is_rook;
};

Bitboard rmask(int);
Bitboard bmask(int);
Bitboard ratt(int, Bitboard);
Bitboard batt(int, Bitboard);
Bitboard map_int_to_mask(Bitboard, int);
Bitboard *find_tile_magic(int, int, Bitboard *, int *);
void print_binary(Bitboard);
int magic_index(Bitboard, Bitboard, Bitboard, int); 
void save_magic_data(const char *, struct magic_tile_response *results[64]);
void load_magic_data(const char *, struct magic_tile_response *results[64]);

uint64_t rnd64(void) {
    static uint64_t state = 88172645463325252ULL;

    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 0x2545F4914F6CDD1DULL;
}

Bitboard random_uint64_fewbits() {
  return rnd64() & rnd64() & rnd64();
}


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


void hash_insert(Hashtable *table, Hash hash, Bitboard move) {
	int table_index = hash % table->size;

	HashEntry *entry = malloc(sizeof(HashEntry));
	entry->hash = hash;
	entry->next = NULL;
	entry->move_bitboard = move;

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
	return -1;  // Not found
}

void hash_clear(Hashtable *table) {
	for (int i = 0; i < table->size; i++) {
		HashEntry *entry = table->table[i];
		while (entry) {
			HashEntry *temp = entry;
			entry = entry->next;
			free(temp);
		}
		table->table[i] = NULL;
	}
}

Bitboard hash_find(Hashtable *table, Hash hash) {
	HashEntry *curr = table->table[hash % table->size];
	while (curr) {
		if (curr->hash == hash) {
			return curr->move_bitboard;
		}
		curr = curr->next;
	}
	return (Bitboard)(-1);  // Not found
}

Bitboard rmask(int sq) {
  Bitboard result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 6; r++) result |= (1ULL << (fl + r*8));
  for(r = rk-1; r >= 1; r--) result |= (1ULL << (fl + r*8));
  for(f = fl+1; f <= 6; f++) result |= (1ULL << (f + rk*8));
  for(f = fl-1; f >= 1; f--) result |= (1ULL << (f + rk*8));
  return result;
}

Bitboard bmask(int sq) {
  Bitboard result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r=rk+1, f=fl+1; r<=6 && f<=6; r++, f++) result |= (1ULL << (f + r*8));
  for(r=rk+1, f=fl-1; r<=6 && f>=1; r++, f--) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl+1; r>=1 && f<=6; r--, f++) result |= (1ULL << (f + r*8));
  for(r=rk-1, f=fl-1; r>=1 && f>=1; r--, f--) result |= (1ULL << (f + r*8));
  return result;
}

Bitboard ratt(int sq, Bitboard block) {
  Bitboard result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1; r <= 7; r++) {
	result |= (1ULL << (fl + r*8));
	if(block & (1ULL << (fl + r*8))) break;
  }
  for(r = rk-1; r >= 0; r--) {
	result |= (1ULL << (fl + r*8));
	if(block & (1ULL << (fl + r*8))) break;
  }
  for(f = fl+1; f <= 7; f++) {
	result |= (1ULL << (f + rk*8));
	if(block & (1ULL << (f + rk*8))) break;
  }
  for(f = fl-1; f >= 0; f--) {
	result |= (1ULL << (f + rk*8));
	if(block & (1ULL << (f + rk*8))) break;
  }
  return result;
}

Bitboard batt(int sq, Bitboard block) {
  Bitboard result = 0ULL;
  int rk = sq/8, fl = sq%8, r, f;
  for(r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
	result |= (1ULL << (f + r*8));
	if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
	result |= (1ULL << (f + r*8));
	if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
	result |= (1ULL << (f + r*8));
	if(block & (1ULL << (f + r * 8))) break;
  }
  for(r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
	result |= (1ULL << (f + r*8));
	if(block & (1ULL << (f + r * 8))) break;
  }
  return result;
}

int magic_index(Bitboard blockers, Bitboard mask, Bitboard magic, int bits) {
	Bitboard relevant = blockers & mask;
	return (int)((relevant * magic) >> (64 - bits));
}

Bitboard map_int_to_mask(Bitboard mask, int value) {
	int one_count = __builtin_popcountll(mask);

	// Check if value exceeds the number of possible configurations
	if (value >= (1 << one_count)) return (Bitboard)(-1);

	Bitboard result = 0ULL;
	int bit_index = 0;

	for (int i = 0; i < 64; i++) {
		if (mask & (1ULL << i)) {
			if (value & (1 << bit_index)) {
				result |= (1ULL << i);
			}
			bit_index++;
		}
	}

	return result;
}

Bitboard *find_tile_magic(int tile_index, int is_rook, Bitboard *magic_bitboard, int *used_bits) {
    Bitboard movable_mask = (is_rook) ? rmask(tile_index) : bmask(tile_index);
    int num_blockers = __builtin_popcountll(movable_mask);
    Hashtable *hash_table = create_hashtable(HASH_TABLE_SIZE);
    int solved = 0;

    while (!solved) {
        for (int bitboard_attempts = 0; bitboard_attempts < CHECK_MAGIC_BITBOARDS; bitboard_attempts++) {
            hash_clear(hash_table);
            *magic_bitboard = random_uint64_fewbits();
            int success = 1;

            for (int i = 0; i < (1 << num_blockers); i++) {
                Bitboard blocker_mask = map_int_to_mask(movable_mask, i);
                Hash key = magic_index(blocker_mask, movable_mask, *magic_bitboard, *used_bits);
                if (hash_find(hash_table, key) != -1) {
                    success = 0;
                    break;
                }
                Bitboard move_mask = (is_rook) ? ratt(tile_index, blocker_mask) : batt(tile_index, blocker_mask);
                hash_insert(hash_table, key, move_mask);
            }

            if (success) {
                solved = 1;
                break;
            }
        }

        if (!solved) {
            (*used_bits)++;  // retry with more bits
        }
    }

    // move hash table into bitboard array of size 2^{used_bits}

    Bitboard *array_of_moves = calloc(pow(2,*used_bits), sizeof(Bitboard));

    HashEntry **table = hash_table->table;
    for (int i = 0; i < hash_table->size; i++) {
    	HashEntry *walk = table[i];
    	if (!walk) continue;
    	while (walk) {
    		// printf("%d - ",walk->hash);
    		// print_binary(walk->move_bitboard);
    		array_of_moves[walk->hash] = walk->move_bitboard;
    		walk = walk->next;
    	}

    }

    delete_hashtable(hash_table);
    return array_of_moves;
}


void print_binary(Bitboard value) {
	for (int i = 63; i >= 0; i--) {
		putchar((value & (1ULL << i)) ? '1' : '0');
	}	
	putchar('\n');
}

void *thread_handler(void *args) {
	struct thread_args *t_args = (struct thread_args * ) args;
	int tile_index = t_args->index;
	int is_rook = t_args->is_rook;
	Bitboard magic_bitboard = 0ULL;
	int bits_used = 5;

	printf("Looking for tile: %d\n", tile_index);
	Bitboard *array = find_tile_magic(tile_index, is_rook, &magic_bitboard, &bits_used);
	printf("Magic information found for tile: %d\n - bits used = %d\n - magic number = %llu\n", tile_index, bits_used, magic_bitboard);

	struct magic_tile_response *resp = malloc(sizeof(struct magic_tile_response));
	resp->bits_used = bits_used;
	resp->magic_bitboard = magic_bitboard;
	resp->array_of_moves = array;

	free(t_args);
	return resp;
}

void save_magic_data(const char *filename, struct magic_tile_response *results[64]) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        exit(1);
    }

    for (int i = 0; i < 64; i++) {
        struct magic_tile_response *res = results[i];
        fwrite(&res->magic_bitboard, sizeof(Bitboard), 1, file);
        fwrite(&res->bits_used, sizeof(int), 1, file);
        size_t table_size = 1ULL << res->bits_used;
        fwrite(res->array_of_moves, sizeof(Bitboard), table_size, file);
    }

    fclose(file);
}


void create_magic_file(const char *filename, int is_rook) {
	pthread_t threads[64];
	struct magic_tile_response *results[64];

	for (int i = 0; i < 64; i++) {
		struct thread_args *arg = malloc(sizeof(struct thread_args));
		arg->index = i;
		arg->is_rook = is_rook;
		pthread_create(&threads[i], NULL, thread_handler, (void *) arg);
	}

    for (int i = 0; i < 64; i++) {
        void *ret_val;
        pthread_join(threads[i], &ret_val);

        results[i] = (struct magic_tile_response *)ret_val;
        printf("Thread %d returned magic bitboard: %llx\n", i, results[i]->magic_bitboard);

    }

    save_magic_data(filename,results);

    for (int i = 0; i < 64; i++) {
    	free(results[i]->array_of_moves);
    	free(results[i]);
    }
}



int main(int argv, char **argc) {
	
	create_magic_file("bishop_magic",0);
	return 0;
}


