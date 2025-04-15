#include "includes/chess.h"

#define LOOK_AHEAD 5


Bitboard CHECK_ALL = 0xFFFFFFFFFFFFFFFF;


static inline int max(int a, int b) {
	return (a > b) ? a : b;
}

static inline int min(int a, int b) {
	return (a < b) ? a : b;
}


Move select_move(Board *state) {
	struct timeval start, end;

	gettimeofday(&start, NULL);

	int pipe_fd[2];
	int children_procs = 0;
	pipe(pipe_fd);

	pid_t pid;

	Bitboard from_mask = 1ULL;
	Bitboard pieces = state->all_pieces[state->active_player];
	for (int i = 0; i < 64; i++) {
		if (pieces & from_mask) {
			children_procs++;
			pid = fork();
			if (pid == 0) {
				close(pipe_fd[0]);
				process_task(*state, pipe_fd[1], from_mask);
			}
		}
		from_mask <<= 1;
	}

	struct alphabeta_response result;
	struct alphabeta_response best = {.score = FLT_MAX, .move = NO_MOVE};


	close(pipe_fd[1]);  // Close write end of the pipe
    for (int i = 0; i < children_procs; i++) {
        read(pipe_fd[0], &result, sizeof(result));  // Read result from the pipe
        // printf("Received result from child %d: %f %d\n", i, result.score, result.move);

        if (result.score < best.score) {
        	best = result;
        }
    }

	// // get the move ADD BACK IN ZOBRIST HASH
	// // struct board_data *data = hash_find(zobrist.hashtable, state->z_hash);

	int from_square, to_square, flags;
	decode_move(best.move, &from_square, &to_square, &flags);
	
	gettimeofday(&end, NULL);
	double time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
	printf("move from %d to %d, score = %.2f took %.2f seconds\n", from_square, to_square, best.score, time);


	return best.move;
}

// in terms of white
float evaluate_board(Board *state) {
	float score = 0;
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

	// score += evaluate_center_control(state, WHITE) - evaluate_center_control(state, BLACK);

	// score += 500 * (state->piece_count[for_player][KING] - state->piece_count[!for_player][KING]);
	score += 100  * (state->piece_count[WHITE][QUEEN] - state->piece_count[BLACK][QUEEN]);
	score += 25  * (state->piece_count[WHITE][ROOK] - state->piece_count[BLACK][ROOK]);
	score += 20   * ((state->piece_count[WHITE][BISHOP] - state->piece_count[BLACK][BISHOP]) +
					 (state->piece_count[WHITE][KNIGHT] - state->piece_count[BLACK][KNIGHT]));
	score += 1   * (state->piece_count[WHITE][PAWN] - state->piece_count[BLACK][PAWN]);

	// make use of number of attack locations
	Bitboard attack_moves_white = generate_attackable_squares(state, WHITE);
	Bitboard attack_moves_black = generate_attackable_squares(state, BLACK);

	attack_score = __builtin_popcountll(attack_moves_white) - __builtin_popcountll(attack_moves_black);


	if (attack_moves_black & state->pieces[WHITE][KING]) {
		check_score -= 1;
	} else if (attack_moves_white & state->pieces[BLACK][KING]) {
		check_score += 1;
	}

	return score + (attack_score * 0.5) + (100 * check_score);
}

void generate_all_moves(Board *state, Move *move_list, int *move_count, Bitboard only_check_mask) {
	// make sure moves less thna MAX_MOVES
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
						mflag = 0;
						if (state->all_pieces[!color] & move_mask) {
							mflag |= CAPTURE;
						}
						if ((state->pieces[color][PAWN] & tile_mask) && (move_mask & 0xFF000000000000FF)) {
							mflag |= PROMOTION;
						}
						if (mflag == 0) {
							mflag = NORMAL;
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
	Move *l_pointer = &move_head[0];
	Move *r_pointer = &move_head[*move_count - 1];
	int l_flag, r_flag;

	while (l_pointer < r_pointer) {
		l_flag = check_move_flag(*l_pointer, NORMAL);
		r_flag = check_move_flag(*r_pointer, CAPTURE | PROMOTION);
		if (l_flag && r_flag) {
			Move temp = *l_pointer;
			*l_pointer = *r_pointer;
			*r_pointer = temp;
			l_pointer++;
			r_pointer--;
		}
		if (!l_flag) {
			l_pointer++;
		}
		if (!r_flag) {
			r_pointer--;
		}
	}
}

void process_task(Board state, int fd, Bitboard inital_tile) {
	// extablish IPC
	// start alpha beta with precondition
	// pipe best move back

	struct alphabeta_response response;
	alphabeta(&state, LOOK_AHEAD, INT_MIN, INT_MAX, FALSE, &response, inital_tile);


	write(fd, &response, sizeof(response));
	exit(0);
}

void alphabeta(Board *state, int depth, int alpha, int beta, int maximize_player, struct alphabeta_response * best_info, Bitboard only_check_mask) {
	// struct board_data *data = hash_find(zobrist.hashtable, state->z_hash);
	float best_score;

	// printf("Depth: %d\n",depth);
	if (depth == 0 || check_flag(state, TERMINAL)) {
		int eval_score = evaluate_board(state);
		best_info->score = eval_score;
		best_info->move = NULL_MOVE;
		return;
	}

	
	Move move_list[MAX_MOVES];
	int move_count, move_status_error;
	struct alphabeta_response response;

	generate_all_moves(state, move_list, &move_count, only_check_mask);

	if (move_count == 0) {
		// printf("NO MOVES (CHECKMATE)\n");
		best_info->score = (maximize_player) ? -1000000 : 1000000;
		best_info->move = NULL_MOVE;
		return;
	}

	if (maximize_player == TRUE) {
		best_score = FLT_MIN;

		for (int i = 0; i < move_count; i++) {
			Board board = *state;
			move_status_error = move_piece(&board, move_list[i]);
			if (move_status_error) { // illegal move
				continue;
			}

			// find max score
			alphabeta(&board, depth - 1, alpha, beta, FALSE, &response, CHECK_ALL);

			if (response.score > best_score) {
				best_info->move = move_list[i];
				best_info->score = response.score;
				best_score = response.score; // this could be removed
			}
			if (best_score > beta) {
				break;
			}

			alpha = max(alpha, best_score);
		}

	} else {
		best_score = FLT_MAX;

		for (int i = 0; i < move_count; i++) {
			Board board = *state;
			move_piece(&board, move_list[i]);

			
			// find min score
			alphabeta(&board, depth - 1, alpha, beta, TRUE, &response, CHECK_ALL);

			if (response.score < best_score) {
				best_info->move = move_list[i];
				best_info->score = response.score;
				best_score = response.score; // this could be removed
			}
			if (best_score < alpha) {
				break;
			}

			beta = min(beta, best_score);
		}
	}
}
