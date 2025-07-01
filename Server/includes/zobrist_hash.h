#ifndef _Z_HASH_H_
#define _Z_HASH_H_

#include "structs.h"

struct board_data *tt_lookup(Hash hash);
void tt_store(Hash hash, int eval_score, int depth, HashFlag flag, Move best_move);
void tt_clear();

#endif