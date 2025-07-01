#include "includes/chess.h"
#include <math.h>

int RBits[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

int BBits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

static Bitboard *find_tile_magic(int, int, Bitboard *, int *);
static void save_magic_data(const char *, MagicData *results[64]);

static Bitboard random_uint64_fewbits() {
  return rnd64() & rnd64() & rnd64();
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

static Bitboard map_int_to_mask(Bitboard mask, int value) {
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

static Bitboard *find_tile_magic(int tile_index, int is_rook, Bitboard *magic_bitboard, int *used_bits) {
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
				Hash key = (Hash) magic_index(blocker_mask, movable_mask, *magic_bitboard, *used_bits);
				if (hash_find(hash_table, key)) {
					success = 0;
					break;
				}
				Bitboard move_mask = (is_rook) ? ratt(tile_index, blocker_mask) : batt(tile_index, blocker_mask);
				Bitboard *data_to_save = malloc(sizeof(Bitboard) * 2);
				// save index first
				// save move map seconds
				data_to_save[0] = key;
				data_to_save[1] = move_mask;
				hash_insert(hash_table, key, (void *) data_to_save);
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
			Bitboard *move_bitboard = (Bitboard *) walk->data;
			array_of_moves[move_bitboard[0]] = move_bitboard[1];
			walk = walk->next;
		}

	}

	delete_hashtable(hash_table);
	return array_of_moves;
}

static void *thread_handler(void *args) {
	struct magic_bitboard_thread_args *t_args = (struct magic_bitboard_thread_args * ) args;
	int tile_index = t_args->index;
	int is_rook = t_args->is_rook;
	Bitboard magic_bitboard = 0ULL;
	int bits_used = is_rook ? RBits[tile_index] : BBits[tile_index];

	printf("Looking for tile: %d\n", tile_index);
	Bitboard *array = find_tile_magic(tile_index, is_rook, &magic_bitboard, &bits_used);
	printf("Magic information found for tile: %d\n - bits used = %d\n - magic number = %llu\n", tile_index, bits_used, magic_bitboard);

	MagicData *resp = malloc(sizeof(MagicData));
	resp->bits_used = bits_used;
	resp->magic_bitboard = magic_bitboard;
	resp->array_of_moves = array;

	free(t_args);
	return resp;
}

static void save_magic_data(const char *filename, MagicData *results[64]) {
	FILE *file = fopen(filename, "wb");
	if (!file) {
		perror("Failed to open file for writing");
		exit(1);
	}

	for (int i = 0; i < 64; i++) {
		MagicData*res = results[i];
		fwrite(&res->magic_bitboard, sizeof(Bitboard), 1, file);
		fwrite(&res->bits_used, sizeof(int), 1, file);
		size_t table_size = 1ULL << res->bits_used;
		fwrite(res->array_of_moves, sizeof(Bitboard), table_size, file);
	}

	fclose(file);
}


void create_magic_file(const char *filename, int is_rook) {
	pthread_t threads[64];
	MagicData *results[64];

	for (int i = 0; i < 64; i++) {
		struct magic_bitboard_thread_args *arg = malloc(sizeof(struct thread_args));
		arg->index = i;
		arg->is_rook = is_rook;
		pthread_create(&threads[i], NULL, thread_handler, (void *) arg);
	}

	for (int i = 0; i < 64; i++) {
		void *ret_val;
		pthread_join(threads[i], &ret_val);

		results[i] = (MagicData *)ret_val;
		printf("Thread %d returned magic bitboard: %llx\n", i, results[i]->magic_bitboard);

	}

	save_magic_data(filename,results);

	for (int i = 0; i < 64; i++) {
		free(results[i]->array_of_moves);
		free(results[i]);
	}
}

void load_magic_file(const char *filename, MagicData *table) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open magic file");
        exit(1);
    }

    for (int i = 0; i < 64; i++) {
        size_t r1 = fread(&table[i].magic_bitboard, sizeof(Bitboard), 1, fp);
        (void)r1;
        size_t r2 = fread(&table[i].bits_used, sizeof(int), 1, fp);
        (void)r2;

        int count = 1 << table[i].bits_used;
        table[i].array_of_moves = malloc(sizeof(Bitboard) * count);
        if (!table[i].array_of_moves) {
            fprintf(stderr, "Failed to allocate array for square %d\n", i);
            exit(1);
        }

        size_t r3 = fread(table[i].array_of_moves, sizeof(Bitboard), count, fp);
        (void)r3;
    }

    fclose(fp);
}

// int main() {
// 	create_magic_file("rook_magic",1);
// 	create_magic_file("bishop_magic",0);
// 	return 0;
// }

