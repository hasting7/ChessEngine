#include "includes/chess.h"

Zobrist zobrist;


void init_zobrist_hash(Zobrist *zobrist, Board *state) {
	for (int color = 0; color < NUM_COLORS; color++) {
		for (int piece = 0; piece < NUM_PIECES; piece++) {
			for (int tile = 0; tile < BOARD_SIZE; tile++) {
				zobrist->board[color][piece][tile] = rnd64();
			}
		}
	}
	zobrist->white_turn_hash = rnd64();
	zobrist->black_turn_hash = rnd64();
	zobrist->hashtable = create_hashtable(HASH_TABLE_SIZE);

	if (state) {
		state->z_hash = get_board_hash(state);
	}
}

// given a board state return the hash value for that state
// THIS DOES NOT MODIFTY THE HASH INSIDE OF THE ZOBRIST OBJ
Hash get_board_hash(Board *state) {
    Hash hash = 0;

    if (state->active_player == BLACK) {
        hash ^= zobrist.black_turn_hash;
    } else {
    	hash ^= zobrist.white_turn_hash;
    }

    for (int i = 0; i < 64; i++) {
        Bitboard mask = 1ULL << i;
        Color piece_color = WHITE;
        Piece piece = NO_PIECE;

        if (state->pieces[WHITE][PAWN] & mask) piece = PAWN;
        else if (state->pieces[WHITE][ROOK] & mask) piece = ROOK;
        else if (state->pieces[WHITE][BISHOP] & mask) piece = BISHOP;
        else if (state->pieces[WHITE][KNIGHT] & mask) piece = KNIGHT;
        else if (state->pieces[WHITE][QUEEN] & mask) piece = QUEEN;
        else if (state->pieces[WHITE][KING] & mask) piece = KING;
        else if (state->pieces[BLACK][PAWN] & mask) { piece = PAWN; piece_color = BLACK; }
        else if (state->pieces[BLACK][ROOK] & mask) { piece = ROOK; piece_color = BLACK; }
        else if (state->pieces[BLACK][BISHOP] & mask) { piece = BISHOP; piece_color = BLACK; }
        else if (state->pieces[BLACK][KNIGHT] & mask) { piece = KNIGHT; piece_color = BLACK; }
        else if (state->pieces[BLACK][QUEEN] & mask) { piece = QUEEN; piece_color = BLACK; }
        else if (state->pieces[BLACK][KING] & mask) { piece = KING; piece_color = BLACK; }

        if (piece != NO_PIECE) {
            hash ^= zobrist.board[piece_color][piece][i];
        }
    }

    return hash;
}

// returns a hash that should be XOR on the exisiting hash to propagate the updates
Hash update_zobrist_move(int from_square, int to_square, Piece piece, Color color) {
	// un apply the pieces hash from inital place
    Hash hash = 0;
	hash ^= zobrist.board[color][piece][from_square];
	// apply the pieces hash of new place
	hash ^= zobrist.board[color][piece][to_square];
    return hash;
}

Hash update_zobrist_capture(int square, Piece piece, Color color) {
	// unapply the pieces hash from the place
    Hash hash = 0;
	hash ^= zobrist.board[color][piece][square];
    return hash;
}

Hash update_zobrist_turn() {
    Hash hash = 0;
	hash ^= zobrist.white_turn_hash;
	hash ^= zobrist.black_turn_hash;
    return hash;
}

Hash update_pawn_promote(int square, Piece to_piece, Color color) {
    Hash hash = 0;
    hash ^= zobrist.board[color][PAWN][square];
    hash ^=zobrist.board[color][to_piece][square];
    return hash;
}

// HashEntry *check_zobrist(Zobrist *zobrist, Hash hash) {
// 	return hash_find(zobrist->hashtable, hash);
// }

// void *zobrist_insert_state(Zobrist *zobrist, Hash hash, int eval_score, int depth) {
// 	hash_insert(zobrist->hashtable, hash, eval_score, depth);

// }



