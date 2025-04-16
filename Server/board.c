#include "includes/chess.h"

Move encode_move(int from_index, int to_index, int flags) {
	// use flags for castling or promotion or en passant
	return (from_index & 0x3F) | ((to_index & 0x3F) << 6) | ((flags & 0xF) << 12);
}

void decode_move(Move move, int *from_index, int *to_index, int *flags) {
	// use flags for castling or promotion or en passant
	*from_index = move & 0x3F;
	*to_index = (move >> 6) & 0x3F;
	*flags = (move >> 12) & 0xF;
}

int check_move_flag(Move move) {
	return ((move >> 12) & 0xF);
}

int is_color_turn(Board *board, Color color) {
	Color turn;
	turn = board->active_player;
	return turn == color;
}

void pawn_promote(Board *state, int pawn_location) {
	Bitboard tile_mask = 1LLU << pawn_location;
	Color player = state->active_player;

	// update hash by removing pawn, placing queen
	state->z_hash ^= update_pawn_promote(pawn_location, QUEEN, state->active_player);

	state->pieces[player][PAWN] &= ~tile_mask;
	state->piece_count[player][PAWN] -= 1;
	state->pieces[player][QUEEN] |= tile_mask;
	state->piece_count[player][QUEEN] += 1;
	state->all_pieces[player] |= tile_mask;

}

// int move_breaks_check(Board *state, Move move) {

// }


int move_piece(Board *state, Move move) {
	if (move == NO_MOVE) {
		return 1;
	}

	int to_index, from_index, flags;
	decode_move(move, &from_index, &to_index, &flags);

	Bitboard from_mask = (1ULL << from_index);
	Bitboard to_mask = (1ULL << to_index);
	Color moving_color = state->active_player;

	// printf("from: %d to: %d flags: %d\n",from_index,to_index,flags);

	// make sure the piece belongs to the active player
	if (!(from_mask & state->all_pieces[moving_color])) {
 		return 1; // not valid
	}
	// make sure the to_index results in a valid place on the moves

	// I AM NOT CHECKING FOR VALID MOVES BUT IM ASSUMING ONLY VALID MOCES CAN BE MADE

	// check if capture or just move

	Hash update_hash = 0;

	Piece piece = piece_on_tile(state, moving_color, from_mask);

	if (piece == PAWN) {
		state->last_capture = 0;
	} else {
		state->last_capture += 1;
	}
	state->move_count += 1;
	if ((1ULL << to_index) & state->all_pieces[!moving_color]) {
		state->last_capture = 0;
		// printf("capture! %d to %d\n",from_index,to_index);

		Piece capture_piece = piece_on_tile(state, !moving_color, to_mask);

		if (capture_piece == KING) {
			set_flag(state, TERMINAL, TRUE);
		}

		update_hash ^= update_zobrist_capture(to_index, capture_piece, !moving_color);

		state->pieces[!moving_color][capture_piece] ^= to_mask;
		state->all_pieces[!moving_color] ^= to_mask;
		state->piece_count[!moving_color][capture_piece] -= 1;

		// update pst for capture
		pst_remove_piece(state, !moving_color, capture_piece, to_index); // remove captured piece
		
	}
	// update pst for normal move
	pst_remove_piece(state, moving_color, piece, from_index); // remove piece being moved;
	
	update_hash ^= update_zobrist_move(from_index, to_index, piece, moving_color);
	

	// update bitboards

	state->pieces[moving_color][piece] ^= from_mask ^ to_mask;
	state->all_pieces[moving_color] ^= from_mask ^ to_mask;

	if (piece == PAWN && (to_mask & 0xFF000000000000FF)) {
		pawn_promote(state, to_index);

		piece = QUEEN;
	}

	pst_add_piece(state, moving_color, piece, to_index); // add piece to to location (after pawn promote just in case)

	// check checks, only check player about to be

	// printf("start checking check\n");
	state->attackable[WHITE] =  generate_attackable_squares(state, WHITE);
	int is_check = (state->attackable[WHITE] & state->pieces[BLACK][KING]) ? 1 : 0;
	set_flag(state, BLACK_CHECK, is_check);

	// printf("is black in check : %d\n",check_flag(state, BLACK_CHECK));

	state->attackable[BLACK] =  generate_attackable_squares(state, BLACK);
	is_check = (state->attackable[BLACK] & state->pieces[WHITE][KING]) ? 1 : 0;
	set_flag(state, WHITE_CHECK, is_check);
//
	// printf("is white in check : %d\n",check_flag(state, WHITE_CHECK));

	// printf("done checking check\n");

	update_hash ^= update_zobrist_turn();

	state->z_hash ^= update_hash;
	state->active_player = !moving_color;
	
	return 0;
}


// create board
Board * create_board() {
	Board * board = malloc(sizeof(Board));

	// initalize bit boards
	board->pieces[WHITE][PAWN] = WHITE_PAWN_START;
	board->pieces[WHITE][ROOK] = WHITE_ROOK_START;
	board->pieces[WHITE][KNIGHT] = WHITE_KNIGHT_START;
	board->pieces[WHITE][BISHOP] = WHITE_BISHOP_START;
	board->pieces[WHITE][KING] = WHITE_KING_START;
	board->pieces[WHITE][QUEEN] = WHITE_QUEEN_START;

	board->pieces[BLACK][PAWN] = BLACK_PAWN_START;
	board->pieces[BLACK][ROOK] = BLACK_ROOK_START;
	board->pieces[BLACK][KNIGHT] = BLACK_KNIGHT_START;
	board->pieces[BLACK][BISHOP] = BLACK_BISHOP_START;
	board->pieces[BLACK][KING] = BLACK_KING_START;
	board->pieces[BLACK][QUEEN] = BLACK_QUEEN_START;

	board->piece_count[WHITE][PAWN]   = __builtin_popcountll(WHITE_PAWN_START);
    board->piece_count[WHITE][KNIGHT] = __builtin_popcountll(WHITE_KNIGHT_START);
    board->piece_count[WHITE][BISHOP] = __builtin_popcountll(WHITE_BISHOP_START);
    board->piece_count[WHITE][ROOK]   = __builtin_popcountll(WHITE_ROOK_START);
    board->piece_count[WHITE][QUEEN]  = __builtin_popcountll(WHITE_QUEEN_START);
    board->piece_count[WHITE][KING]   = __builtin_popcountll(WHITE_KING_START);

    board->piece_count[BLACK][PAWN]   = __builtin_popcountll(BLACK_PAWN_START);
    board->piece_count[BLACK][KNIGHT] = __builtin_popcountll(BLACK_KNIGHT_START);
    board->piece_count[BLACK][BISHOP] = __builtin_popcountll(BLACK_BISHOP_START);
    board->piece_count[BLACK][ROOK]   = __builtin_popcountll(BLACK_ROOK_START);
    board->piece_count[BLACK][QUEEN]  = __builtin_popcountll(BLACK_QUEEN_START);
    board->piece_count[BLACK][KING]   = __builtin_popcountll(BLACK_KING_START);

	board->all_pieces[WHITE] = WHITE_PAWN_START | WHITE_ROOK_START | WHITE_KNIGHT_START | WHITE_BISHOP_START | WHITE_KING_START | WHITE_QUEEN_START;
	board->all_pieces[BLACK] = BLACK_PAWN_START | BLACK_ROOK_START | BLACK_KNIGHT_START | BLACK_BISHOP_START | BLACK_KING_START | BLACK_QUEEN_START;

	board->attackable[WHITE] = 0LLU;
	board->attackable[BLACK] = 0LLU;
	// initalize hash
	board->active_player = WHITE;
	board->move_count = 2;
	board->last_capture = 0;
	board->flags = 0;
	board->pst_scores[MIDGAME] = 0;
	board->pst_scores[ENDGAME] = 0;
	initialize_pst(board);

	return board;
};

Piece piece_on_tile(Board *state, Color color_of_piece, uint64_t mask) {
	for (int piece = PAWN; piece <= KING; piece++) {
        if (mask & state->pieces[color_of_piece][piece]) {
            return piece;
        }
    }

    return NO_PIECE;
}


// disbatcher to the individual move functions per pieces
Bitboard generate_moves(Board *state, int tile_index, int account_for_check) {

	Bitboard from_mask = 1ULL << tile_index;

	// check if piece is in the tile
	if (!(from_mask & (state->all_pieces[WHITE] | state->all_pieces[BLACK]))) {
		return NO_MOVE;
	}

	Color color = (from_mask & state->all_pieces[WHITE]) ? WHITE : BLACK;
	Bitboard moves;

	switch (piece_on_tile(state, color, from_mask)) {
		case PAWN:
			moves = generate_pawn_moves(state, from_mask, color);
			break;
		case KNIGHT:
			moves = generate_knight_moves(state, from_mask, color);
			break;
		case ROOK:
			moves = generate_rook_moves(state, from_mask, color);
			break;
		case BISHOP:
			moves = generate_bishop_moves(state, from_mask, color);
			break;
		case QUEEN:
			moves = generate_queen_moves(state, from_mask, color);
			break;
		case KING:
			moves = generate_king_moves(state, from_mask, color);
			break;
		default:
			moves = NO_MOVE;
			break;
	}

	// add check check
	BoardFlag flag = (color == WHITE) ? WHITE_CHECK : BLACK_CHECK;
	if (moves && account_for_check) {
		// Bitboard copy = moves;
		Bitboard to_mask = 1ULL;
		Move check_move;
		Board temp_board;
		int offset = 0;

		for (; offset < 64; offset++) {
			if (to_mask & moves) {
				check_move = encode_move(tile_index, offset, 0);
				temp_board = *state;
				move_piece(&temp_board, check_move);

				if (check_flag(&temp_board, flag)) {
					moves ^= to_mask;
				}
			}
			to_mask <<= 1;
		}
		// printf("%s in check\n",(state->active_player) ? "black" : "white");
	}
	return moves;
}

Bitboard generate_knight_moves(Board *state, Bitboard knight, Color color) {
   uint64_t l1 = (knight >> 1) & 0x7f7f7f7f7f7f7f7f;
   uint64_t l2 = (knight >> 2) & 0x3f3f3f3f3f3f3f3f;
   uint64_t r1 = (knight << 1) & 0xfefefefefefefefe;
   uint64_t r2 = (knight << 2) & 0xfcfcfcfcfcfcfcfc;
   uint64_t h1 = l1 | r1;
   uint64_t h2 = l2 | r2;
   return ((h1<<16) | (h1>>16) | (h2<<8) | (h2>>8)) & (~state->all_pieces[color]);
}

Bitboard generate_pawn_moves(Board *state, Bitboard pawn, Color color) {
    uint64_t empty = ~(state->all_pieces[WHITE] | state->all_pieces[BLACK]);
    uint64_t push_moves, attack_moves, single_move, double_move;
    if (color == WHITE) {
    	//0x000000000000FF00ULL
        single_move = (pawn << 8) & empty;
        double_move = ((single_move & 0x0000000000FF0000ULL) << 8) & empty;
        push_moves = single_move | double_move;
        attack_moves = ((pawn & NOT_A_FILE) << 7 | (pawn & NOT_H_FILE) << 9) & state->all_pieces[BLACK];
    } else {
    	//0x00FF000000000000ULL
        single_move = (pawn >> 8) & empty;
        double_move = ((single_move & 0x0000FF0000000000ULL) >> 8) & empty;
        push_moves = single_move | double_move;
        attack_moves = ((pawn & NOT_H_FILE) >> 7 | (pawn & NOT_A_FILE) >> 9) & state->all_pieces[WHITE];
    }
    
    return push_moves | attack_moves;
}

Bitboard generate_rook_moves(Board *state, Bitboard rook, Color color) {
    uint64_t occupied = state->all_pieces[WHITE] | state->all_pieces[BLACK];
    uint64_t enemy_pieces = state->all_pieces[!color];
    uint64_t possible = 0;

    uint64_t ray = rook;
    while (ray & NOT_H_FILE) {
        ray <<= 1;
        possible |= ray;
        if (ray & occupied) {
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = rook;
    while (ray & NOT_A_FILE) {
        ray >>= 1;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = rook;
    while (ray) {
        ray <<= 8;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = rook;
    while (ray) {
        ray >>= 8;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }

    return possible;
}

Bitboard generate_bishop_moves(Board *state, Bitboard bishop, Color color) {
    uint64_t occupied = state->all_pieces[WHITE] | state->all_pieces[BLACK];
    uint64_t enemy_pieces = state->all_pieces[!color];
    uint64_t possible = 0;

    uint64_t ray = bishop;
    while (ray & NOT_A_FILE) {
        ray <<= 7;
        possible |= ray;
        if (ray & occupied) {
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = bishop;
    while (ray & NOT_H_FILE) {
        ray >>= 7;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = bishop;
    while (ray & NOT_H_FILE) {
        ray <<= 9;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }
    ray = bishop;
    while (ray & NOT_A_FILE) {
        ray >>= 9;
        possible |= ray;
        if (ray & occupied) { 
            possible &= ~ray;
            if (ray & enemy_pieces) possible |= ray;
            break;
        }
    }

    return possible;
}

Bitboard generate_queen_moves(Board *state, Bitboard queen, Color color) {
	return generate_rook_moves(state, queen, color) | generate_bishop_moves(state, queen, color);
}

Bitboard generate_king_moves(Board *state, Bitboard king, Color color) {
	uint64_t possible = 0;
	possible |= (king << 8) & ~state->all_pieces[color];
	possible |= (king >> 8) & ~state->all_pieces[color];
	possible |= (king << 1) & ~state->all_pieces[color] & NOT_A_FILE;
	possible |= (king >> 1) & ~state->all_pieces[color] & NOT_H_FILE; 
	possible |= (king << 7) & ~state->all_pieces[color] & NOT_H_FILE;
	possible |= (king << 9) & ~state->all_pieces[color] & NOT_A_FILE;
	possible |= (king >> 7) & ~state->all_pieces[color] & NOT_A_FILE;
	possible |= (king >> 9) & ~state->all_pieces[color] & NOT_H_FILE; 
   return possible;
}

Bitboard generate_attackable_squares(Board *state, Color color) {
	Bitboard total_moves = 0LLU;
	Bitboard mask = 1;
	int from_index = 0; // how can I add more value to this?
	for (; from_index < 64; from_index++) {
		if (mask & state->all_pieces[color]) {
			total_moves |= generate_moves(state, from_index, FALSE);
		}
		mask = mask << 1;
	}
	return total_moves;
}


char *generate_moves_as_string(Board *state, int tile_index) {
	uint64_t moves_list = generate_moves(state, tile_index, TRUE);
	// uint64_t moves_list = generate_attackable_squares(state, state->active_player);
	char buffer[1024] = { 0 };
	int len = 0;
	char rank;
	char file;

	if (moves_list == NO_MOVE) {
		// idk what to do right now but this is if no moves are valid just pass that back to client
		buffer[len++] = '-';
		buffer[len++] = '1';
	} else {
		uint64_t mask = 1ULL;
		for (int i = 0; i < 64; i++) {
			if (moves_list & mask) {
				printf("to_index: %d\n",i);
				file = ('h' - (i % 8));
				rank = i / 8 + '1';
				buffer[len++] = file;
				buffer[len++] = rank;
				buffer[len++] = ',';
			}
			mask = mask << 1;
		}
		buffer[--len] = '\0'; // remove last ','
	}


	char *return_string = calloc(len, sizeof(char));
	strncpy(return_string, buffer, len);
	return_string[len] = '\0';

	return return_string;
}

int check_flag(Board *state, BoardFlag flag) {
	return ((1 << flag) & state->flags) ? 1 : 0;
}

void set_flag(Board *state, BoardFlag flag, int flag_value) {
	if (check_flag(state, flag) != flag_value) { // the flag is needs to be toggled
		state->flags ^= (1 << flag);
	}
}

char *board_to_fen(Board *state) {

	char fen_string[1024] = { 0 };
	int fen_len = 0;
	char piece_char = '\0';
	int empty_run = 0;
	int row_spaces_counted = 0;
	char run_length_char;

	Bitboard iterator = (1ULL << 63);
	for (int i = 63; i >= 0; i--) {
		if (state->all_pieces[WHITE] & iterator) {
			switch (piece_on_tile(state, WHITE, iterator)) {
				case PAWN:
					piece_char = WHITE_PAWN;
					break;
				case ROOK:
					piece_char = WHITE_ROOK;
					break;
				case KNIGHT:
					piece_char = WHITE_KNIGHT;
					break;
				case BISHOP:
					piece_char = WHITE_BISHOP;
					break;
				case QUEEN:
					piece_char = WHITE_QUEEN;
					break;
				case KING:
					piece_char = WHITE_KING;
					break;
				case NO_PIECE:
					break;
			}

		} else if (state->all_pieces[BLACK] & iterator) {
			
			switch (piece_on_tile(state, BLACK, iterator)) {
				case PAWN:
					piece_char = BLACK_PAWN;
					break;
				case ROOK:
					piece_char = BLACK_ROOK;
					break;
				case KNIGHT:
					piece_char = BLACK_KNIGHT;
					break;
				case BISHOP:
					piece_char = BLACK_BISHOP;
					break;
				case QUEEN:
					piece_char = BLACK_QUEEN;
					break;
				case KING:
					piece_char = BLACK_KING;
					break;
				case NO_PIECE:
					break;
			}
		}

		if (row_spaces_counted == 8) {
			row_spaces_counted = 0;
			if (empty_run) {
				run_length_char = 48 + empty_run;
				fen_string[fen_len++] = run_length_char;

				empty_run = 0;
			}
			fen_string[fen_len++] = '/';
		}


		if (piece_char) {
			if (empty_run) {
				char run_length_char = 48 + empty_run;
				fen_string[fen_len++] = run_length_char;

				empty_run = 0;
			}
			// a piece exists
			fen_string[fen_len++] = piece_char;
			piece_char = '\0';
		} else {
			// no piece
			empty_run += 1;
		}

		row_spaces_counted += 1;
		iterator >>= 1;
	}
	if (empty_run) {
		run_length_char = 48 + empty_run;
		fen_string[fen_len++] = run_length_char;
	}

	// add in the extra stuff
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = (state->active_player == WHITE) ? WHITE_STR : BLACK_STR;
	fen_string[fen_len++] = ' ';
	// castling
	fen_string[fen_len++] = '-';
	fen_string[fen_len++] = ' ';
	//en passent
	fen_string[fen_len++] = '-';
	fen_string[fen_len++] = ' ';

	int half_move = state->last_capture;
	int full_move = state->move_count / 2;
	char buffer[10];
	char *curr_char;
	int buff_len = 0;
	int display_char;

	if (half_move == 0) {
		fen_string[fen_len++] = '0';
	} else {
		while (half_move > 0) {
			display_char = half_move % 10;
			half_move = half_move / 10;
			buffer[buff_len++] = '0' + display_char;
		}
		curr_char = &buffer[buff_len-1];
		while (&buffer[0] <= curr_char) {
			fen_string[fen_len++] = *(curr_char--);

		}
	}
	fen_string[fen_len++] = ' ';

	// full move
	
	buff_len = 0;

	while (full_move > 0) {
		display_char = full_move % 10;
		full_move = full_move / 10;
		buffer[buff_len++] = '0' + display_char;
	}
	curr_char = &buffer[buff_len-1];
	while (&buffer[0] <= curr_char) {
		fen_string[fen_len++] = *(curr_char--);

	}


	char *return_string = malloc(sizeof(char) * (fen_len + 1));
	strncpy(return_string, fen_string, fen_len);
	return_string[fen_len] = '\0';

	return return_string;
}

