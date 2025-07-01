#include "includes/chess.h"
#include "includes/thread_pool.h"


// before magic bitboards
// 6 lookahead
// 	move 1: ~6 sec
//	move 2: ~21 sec
//	move 3: ~30 sec

// after magic bitboard

#define LOOK_AHEAD 6


Bitboard CHECK_ALL = 0xFFFFFFFFFFFFFFFF;


int generated_boards = 0;
int checked_boards = 0;

static inline int max(int a, int b) {
	return (a > b) ? a : b;
}

static inline int min(int a, int b) {
	return (a < b) ? a : b;
}


int capture_value[6] = {
    100,  // PAWN
    500,  // ROOK
    320,  // KNIGHT
    330,  // BISHOP
    900,  // QUEEN
    0     // KING
};

#define POOL_THREADS 4
static ThreadPool pool;
static int pool_initialized = 0;

struct search_task {
    Board board;
    Bitboard tile;
    struct alphabeta_response result;
};

static void search_task_run(void *arg) {
    struct search_task *task = (struct search_task *)arg;
    alphabeta(&task->board, LOOK_AHEAD, INT_MIN, INT_MAX, FALSE, &task->result, task->tile);
}


Move select_move(Board *state) {
        struct timeval start, end;

        if (!pool_initialized) {
                pool_init(&pool, POOL_THREADS);
                pool_initialized = 1;
        }

        gettimeofday(&start, NULL);

        PoolTask tasks[64];
        struct search_task stasks[64];
        int task_count = 0;

        Bitboard from_mask = 1ULL;
        Bitboard pieces = state->all_pieces[state->active_player];
        for (int i = 0; i < 64; i++) {
                if (pieces & from_mask) {
                        stasks[task_count].board = *state;
                        stasks[task_count].tile = from_mask;
                        tasks[task_count].func = search_task_run;
                        tasks[task_count].arg = &stasks[task_count];
                        pool_add_task(&pool, &tasks[task_count]);
                        task_count++;
                }
                from_mask <<= 1;
        }

        struct alphabeta_response best = {.score = INT_MAX, .move = NO_MOVE};
        for (int i = 0; i < task_count; i++) {
                pool_wait_task(&tasks[i]);
                if (stasks[i].result.score < best.score) {
                        best = stasks[i].result;
                }
        }

	// // get the move ADD BACK IN ZOBRIST HASH
	// // struct board_data *data = hash_find(zobrist.hashtable, state->z_hash);

	int from_square, to_square, flags;
	decode_move(best.move, &from_square, &to_square, &flags);
	
	gettimeofday(&end, NULL);
	double time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
	printf("move from %d to %d, score = %d took %.2f seconds\n", from_square, to_square, best.score, time);
	printf("PST\n\tmidgame: %d\n\tendgame: %d\n",board->pst_scores[MIDGAME],board->pst_scores[ENDGAME]);

	return best.move;
}

// in terms of white
int evaluate_board(Board *state) {
	int score = 0;
	int attack_score = 0;
	int check_score = 0;


	if (state->piece_count[WHITE][KING] == 0) {
		set_flag(state, TERMINAL, TRUE);
		return -10000;
	}
	if (state->piece_count[BLACK][KING] == 0) {
		set_flag(state, TERMINAL, TRUE);
		return 10000;
	}

	if (state->last_capture >= 100) {
		set_flag(state, TERMINAL, TRUE);
		return 0;
	}

	score += evaluate_pieces(state);

	// make use of number of attack locations
	Bitboard attack_moves_white = generate_attackable_squares(state, WHITE);
	Bitboard attack_moves_black = generate_attackable_squares(state, BLACK);

	attack_score = __builtin_popcountll(attack_moves_white) - __builtin_popcountll(attack_moves_black);


	if (attack_moves_black & state->pieces[WHITE][KING]) {
		check_score -= 1;
	} else if (attack_moves_white & state->pieces[BLACK][KING]) {
		check_score += 1;
	}

	return score + (attack_score * 2) + (200 * check_score);
}

int sort_key(const void *a, const void *b) {
	Move *m_a = (Move *) a;
	Move *m_b = (Move *) b;
	return check_move_flag(*m_b) - check_move_flag(*m_a);

}

void generate_all_moves(Board *state, Move *move_list, int *move_count, Bitboard only_check_mask) {
        // make sure moves less than MAX_MOVES
        Move *move_head = move_list;
        *move_count = 0;

	Color color = state->active_player;

	int to_index, from_index;
	Bitboard tile_mask, move_mask, valid_moves;
	int mflag;
	// Board temp_state;
	Move possible_move;

	tile_mask = 1ULL;
	from_index = 0;

	// int is_in_check = check_flag(state, () state->active_player);

	for (; from_index < 64; from_index++) {
		if (state->all_pieces[color] & tile_mask & only_check_mask) {
			// piece is here;
			valid_moves = generate_moves(state, from_index, TRUE);
			if (valid_moves) {
				move_mask = 1ULL;
				to_index = 0;
				for (; to_index < 64; to_index++) {
					if (valid_moves & move_mask && *move_count < MAX_MOVES) {
						// HERE WE HAVE VALID FROM AND TO INDEXS
						mflag = 0x0;
						// flags on range from 0-15 with 15 best move
						if (state->all_pieces[!color] & move_mask) { // capture
							Piece victim = piece_on_tile(state,!color,move_mask);
							Piece attacker = piece_on_tile(state, color, tile_mask);
							int score = (capture_value[victim] * 10 - capture_value[attacker]) / 600; // 575 is arbitrary to get 8900 to between 0 amd 15
							// score is [0, 8900]
							// determine bucket
							mflag = score + 1;
						}
						if ((state->pieces[color][PAWN] & tile_mask) && (move_mask & 0xFF000000000000FF)) {
							mflag = 0xf;
						}

						possible_move = encode_move(from_index, to_index, mflag);

						*move_list = possible_move;
						move_list++;

						(*move_count)++;
					}
					move_mask <<= 1;
				}
			}
		}
		tile_mask <<= 1;
	}
        if (*move_count > 1) {
                qsort(move_head, *move_count, sizeof(Move), sort_key);
        }
}


void alphabeta(Board *state, int depth, int alpha, int beta, int maximize_player, struct alphabeta_response * best_info, Bitboard only_check_mask) {
        Hash board_hash = state->z_hash;
        int best_score;
        int alpha_orig = alpha;
        int beta_orig = beta;

        struct board_data *entry = hash_find(zobrist.hashtable, board_hash);
        if (entry && entry->depth >= depth) {
                if (entry->flags == EXACT) {
                        best_info->score = entry->eval_score;
                        best_info->move = entry->best_move;
                        return;
                } else if (entry->flags == LOWER) {
                        alpha = max(alpha, entry->eval_score);
                } else if (entry->flags == UPPER) {
                        beta = min(beta, entry->eval_score);
                }

                if (alpha >= beta) {
                        best_info->score = entry->eval_score;
                        best_info->move = entry->best_move;
                        return;
                }
        }

	// printf("Depth: %d\n",depth);
        if (depth == 0 || check_flag(state, TERMINAL)) {
                int eval_score = evaluate_board(state);
                best_info->score = eval_score;
                best_info->move = 0;

                struct board_data data = { .eval_score = eval_score, .depth = depth, .flags = EXACT, .best_move = 0 };
                hash_insert(zobrist.hashtable, board_hash, data);
                return;
        }

	
	Move move_list[MAX_MOVES];
	int move_count, move_status_error;
	struct alphabeta_response response;


	generate_all_moves(state, move_list, &move_count, only_check_mask);

	if (move_count == 0) {
		// printf("NO MOVES (CHECKMATE)\n");
                best_info->score = (maximize_player) ? -1000000 : 1000000;
                best_info->move = 0;
		return;
	}

        Move best_move_local = 0;

        if (maximize_player == TRUE) {
                best_score = INT_MIN;

                for (int i = 0; i < move_count; i++) {
                        Board board = *state;
                        move_status_error = move_piece(&board, move_list[i]);
                        if (move_status_error) {
                                continue;
                        }
                        checked_boards += 1;

                        alphabeta(&board, depth - 1, alpha, beta, FALSE, &response, CHECK_ALL);

                        if (response.score > best_score) {
                                best_move_local = move_list[i];
                                best_score = response.score;
                        }

                        alpha = max(alpha, best_score);
                        if (alpha >= beta) break;
                }

        } else {
                best_score = INT_MAX;

                for (int i = 0; i < move_count; i++) {
                        Board board = *state;
                        move_status_error = move_piece(&board, move_list[i]);
                        if (move_status_error) {
                                continue;
                        }


                        alphabeta(&board, depth - 1, alpha, beta, TRUE, &response, CHECK_ALL);

                        if (response.score < best_score) {
                                best_move_local = move_list[i];
                                best_score = response.score;
                        }

                        beta = min(beta, best_score);
                        if (alpha >= beta) break;
                }
        }

        best_info->move = best_move_local;
        best_info->score = best_score;

        HashFlag flag;
        if (best_score <= alpha_orig) {
                flag = UPPER;
        } else if (best_score >= beta_orig) {
                flag = LOWER;
        } else {
                flag = EXACT;
        }

        struct board_data new_entry = { .eval_score = best_score, .depth = depth, .flags = flag, .best_move = best_move_local };
        hash_insert(zobrist.hashtable, board_hash, new_entry);
}
