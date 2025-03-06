#include "includes/chess.h"

/*

IMPORTANT
when movign a piece i need to update the zobrist hash using the functiosn
update the bitboard of the piece type
update the bitboard of the all pieces

*/

//return (1ULL << to_tile_index) & avaliable_moves;



int move_piece(Board *state, Zobrist *zobrist, int from_index, int to_index, Bitboard valid_moves, Color player_color) {
	Bitboard from_mask = (1ULL << from_index);
	Bitboard to_mask = (1ULL << to_index);
	Color moving_color = state->active_player;
	// make sure the piece belongs to the active player
	if (!(from_mask & state->all_pieces[moving_color]) || moving_color != player_color) {
 		return 1; // not valid
	}
	// make sure the to_index results in a valid place on the moves
	if (!(to_mask & valid_moves)) {
		return 1;
	}

	// the from_index is a valid piece of the active player
	// the to_index in a valid move option

	// check if capture or just move

	Piece piece = piece_on_tile(state, moving_color, from_mask);

	if ((1ULL << to_index) & state->all_pieces[!moving_color]) {
		printf("capture! %d to %d\n",from_index,to_index);

		Piece capture_piece = piece_on_tile(state, moving_color, from_mask);

		update_zobrist_capture(zobrist, to_index, capture_piece, !moving_color);

		state->pieces[!moving_color][capture_piece] ^= to_mask;
		state->all_pieces[!moving_color] ^= to_mask;
	}
	update_zobrist_move(zobrist, from_index, to_index, piece, moving_color);

	// update bitboards

	state->pieces[moving_color][piece] ^= from_mask ^ to_mask;
	state->all_pieces[moving_color] ^= from_mask ^ to_mask;

	update_zobrist_turn(zobrist);
	state->active_player = !state->active_player;


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

	board->all_pieces[WHITE] = WHITE_PAWN_START | WHITE_ROOK_START | WHITE_KNIGHT_START | WHITE_BISHOP_START | WHITE_KING_START | WHITE_QUEEN_START;
	board->all_pieces[BLACK] = BLACK_PAWN_START | BLACK_ROOK_START | BLACK_KNIGHT_START | BLACK_BISHOP_START | BLACK_KING_START | BLACK_QUEEN_START;

	// initalize hash
	board->active_player = WHITE;
	board->move_count = 0;
	board->last_capture = 0;

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
uint64_t generate_moves(Board *state, int tile_index) {

	uint64_t mask = 1ULL << tile_index;

	Piece piece = NO_PIECE;

	if (mask & state->all_pieces[WHITE]) {
		switch (piece_on_tile(state, WHITE, mask)) {
			case PAWN:
				return generate_pawn_moves(state, mask, WHITE);
				break;
			case KNIGHT:
				return generate_knight_moves(state, mask, WHITE);
				break;
			case ROOK:
				return generate_rook_moves(state, mask, WHITE);
				break;
			case BISHOP:
				return generate_bishop_moves(state, mask, WHITE);
				break;
			case QUEEN:
				return generate_queen_moves(state, mask, WHITE);
				break;
			case KING:
				return generate_king_moves(state, mask, WHITE);
				break;
			default:
				return NO_MOVE;
				break;
		}
	} else if (mask & state->all_pieces[BLACK]) {
		switch (piece_on_tile(state, BLACK, mask)) {
			case PAWN:
				return generate_pawn_moves(state, mask, BLACK);
				break;
			case KNIGHT:
				return generate_knight_moves(state, mask, BLACK);
				break;
			case ROOK:
				return generate_rook_moves(state, mask, BLACK);
				break;
			case BISHOP:
				return generate_bishop_moves(state, mask, BLACK);
				break;
			case QUEEN:
				return generate_queen_moves(state, mask, BLACK);
				break;
			case KING:
				return generate_king_moves(state, mask, BLACK);
				break;
			default:
				return NO_MOVE;
				break;
		}
	}
	printf("%d\n",piece);

	return 0;
}

uint64_t generate_knight_moves(Board *state, uint64_t knight, Color color) {
   uint64_t l1 = (knight >> 1) & 0x7f7f7f7f7f7f7f7f;
   uint64_t l2 = (knight >> 2) & 0x3f3f3f3f3f3f3f3f;
   uint64_t r1 = (knight << 1) & 0xfefefefefefefefe;
   uint64_t r2 = (knight << 2) & 0xfcfcfcfcfcfcfcfc;
   uint64_t h1 = l1 | r1;
   uint64_t h2 = l2 | r2;
   return ((h1<<16) | (h1>>16) | (h2<<8) | (h2>>8)) & (~state->all_pieces[color]);
}

uint64_t generate_pawn_moves(Board *state, uint64_t pawn, Color color) {
	uint64_t single_move = (pawn << 8) >> (color << 4);
	uint64_t pawn_home_rank = 0xff00ULL << ((color == WHITE) ? 0 : 40);
	uint64_t double_move = (color == WHITE) ? (pawn << 16) : (pawn >> 16);
	double_move = (pawn & pawn_home_rank) ? double_move : 0;

	uint64_t empty = ~(state->all_pieces[WHITE] | state->all_pieces[BLACK]);
	uint64_t push_moves = (single_move | double_move) & empty;
	uint64_t attack_moves = ((color == WHITE) ? ((pawn << 7 | pawn << 9) & state->all_pieces[BLACK]) : ((pawn >> 7 | pawn >> 9) & state->all_pieces[WHITE]));
   return push_moves | attack_moves;
}

uint64_t generate_rook_moves(Board *state, uint64_t rook, Color color) {
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

uint64_t generate_bishop_moves(Board *state, uint64_t bishop, Color color) {
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

uint64_t generate_queen_moves(Board *state, uint64_t queen, Color color) {
	return generate_rook_moves(state, queen, color) | generate_bishop_moves(state, queen, color);
}

uint64_t generate_king_moves(Board *state, uint64_t king, Color color) {
   uint64_t possible = 0;
   possible |= (king << 8) & ~state->all_pieces[color];
   possible |= (king >> 8) & ~state->all_pieces[color];
   possible |= (king << 1) & ~state->all_pieces[color];
   possible |= (king >> 1) & ~state->all_pieces[color];
   possible |= (king << 7) & ~state->all_pieces[color];
   possible |= (king << 9) & ~state->all_pieces[color];
   possible |= (king >> 7) & ~state->all_pieces[color];
   possible |= (king >> 9) & ~state->all_pieces[color];
   return possible;
}


char *generate_moves_as_string(Board *state, int tile_index) {
	uint64_t moves_list = generate_moves(state, tile_index);
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
				rank = (i % 8) + 49;
				file = i / 8 + 97;
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


char *board_to_fen(Board *state) {

	char fen_string[1024] = { 0 };
	int fen_len = 0;
	char piece_char = '\0';
	int empty_run = 0;
	int row_spaces_counted = 0;
	char run_length_char;

	Bitboard iterator = 1ULL;
	for (int i = 0; i < 64; i++) {
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
		iterator = iterator << 1;
	}
	if (empty_run) {
		run_length_char = 48 + empty_run;
		fen_string[fen_len++] = run_length_char;
	}

	// add in the extra stuff
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = (state->active_player == WHITE) ? WHITE_STR : BLACK_STR;
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = '-';
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = '-';
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = '-';
	fen_string[fen_len++] = ' ';
	fen_string[fen_len++] = '-';


	char *return_string = malloc(sizeof(char) * (fen_len + 1));
	strncpy(return_string, fen_string, fen_len);
	return_string[fen_len] = '\0';

	return return_string;
}

