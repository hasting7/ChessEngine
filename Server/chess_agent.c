#include "includes/chess.h"

#define CENTER_SQUARES 0x0000003c3c000000
#define CENTER_PENALTY 5

static inline int max(int a, int b) {
	return (a > b) ? a : b;
}

static inline int min(int a, int b) {
	return (a < b) ? a : b;
}

Move select_move(Board *state) {
	struct alphabeta_response response;
	alphabeta(state, 5, INT_MIN, INT_MAX, FALSE, &response);

	// get the move ADD BACK IN ZOBRIST HASH
	// struct board_data *data = hash_find(zobrist.hashtable, state->z_hash);

	Move move = response.move;

	if (move == NO_MOVE) {
		printf("CHECKMATE");
		return NO_MOVE;
	}

	int from_square, to_square, flags;
	decode_move(move, &from_square, &to_square, &flags);
	printf("move from %d to %d, score = %.2f\n", from_square, to_square, response.score);

	return move;
}

int evaluate_center_control(Board *state, Color color) {
    uint64_t control = state->attackable[color] & CENTER_SQUARES;
    int control_score = __builtin_popcountll(control) * CENTER_PENALTY; // Count how many center squares are controlled
    return control_score;
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

	score += evaluate_center_control(state, WHITE) - evaluate_center_control(state, BLACK);

	// score += 500 * (state->piece_count[for_player][KING] - state->piece_count[!for_player][KING]);
	score += 25  * (state->piece_count[WHITE][QUEEN] - state->piece_count[BLACK][QUEEN]);
	score += 13  * (state->piece_count[WHITE][ROOK] - state->piece_count[BLACK][ROOK]);
	score += 10   * ((state->piece_count[WHITE][BISHOP] - state->piece_count[BLACK][BISHOP]) +
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

	return score + (attack_score * 0.1) + (1000 * check_score);
}

void generate_all_moves(Board *state, Move *move_list, int *move_count) {
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
		if (state->all_pieces[color] & tile_mask) {
			// piece is here;
			valid_moves = generate_moves(state, from_index, TRUE);
			if (valid_moves) {
				move_mask = 1ULL;
				to_index = 0;
				for (; to_index < 64; to_index++) {
					if (valid_moves & move_mask && *move_count < MAX_MOVES) {
						// HERE WE HAVE VALID FROM AND TO INDEXS
						mflag = NORMAL;
						if (state->all_pieces[!color] & move_mask) {
							mflag = CAPTURE;
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
		r_flag = check_move_flag(*r_pointer, CAPTURE);
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


void alphabeta(Board *state, int depth, int alpha, int beta, int maximize_player, struct alphabeta_response * best_info) {
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

	generate_all_moves(state, move_list, &move_count);

	if (move_count == 0) {
		printf("NO MOVES (CHECKMATE)\n");
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
			alphabeta(&board, depth - 1, alpha, beta, FALSE, &response);

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
			alphabeta(&board, depth - 1, alpha, beta, TRUE, &response);

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
