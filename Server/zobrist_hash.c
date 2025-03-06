#include "includes/chess.h"

int seed = 0;

Zobrist zobrist;

int get_seed() {
    if (seed == 0) {
        seed = (int) time(NULL);
    }
   	seed++;
    return seed;
}

uint64_t rnd64() {

    const uint64_t z = 0x9FB21C651E98DF25;

    uint64_t n = get_seed();

    n ^= ((n << 49) | (n >> 15)) ^ ((n << 24) | (n >> 40));
    n *= z;
    n ^= n >> 35;
    n *= z;
    n ^= n >> 28;

    return n;
}

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
	zobrist->hash = 0;
	zobrist->hashtable = create_hashtable(HASH_TABLE_SIZE);

	if (state) {
		zobrist->hash = get_board_hash(zobrist, state);
	}
}

// given a board state return the hash value for that state
// THIS DOES NOT MODIFTY THE HASH INSIDE OF THE ZOBRIST OBJ
Hash get_board_hash(Zobrist *zobrist, Board *state) {
    Hash hash = 0;

    if (state->active_player == BLACK) {
        hash ^= zobrist->black_turn_hash;
    } else {
    	hash ^= zobrist->white_turn_hash;
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
            hash ^= zobrist->board[piece_color][piece][i];
        }
    }

    return hash;
}


void update_zobrist_move(Zobrist *zobrist, int from_square, int to_square, Piece piece, Color color) {
	// un apply the pieces hash from inital place
	zobrist->hash ^= zobrist->board[color][piece][from_square];
	// apply the pieces hash of new place
	zobrist->hash ^= zobrist->board[color][piece][to_square];

}

void update_zobrist_capture(Zobrist *zobrist, int square, Piece piece, Color color) {
	// unapply the pieces hash from the place
	zobrist->hash ^= zobrist->board[color][piece][square];
}

void update_zobrist_turn(Zobrist *zobrist) {
	zobrist->hash ^= zobrist->white_turn_hash;
	zobrist->hash ^= zobrist->black_turn_hash;
}

// HashEntry *check_zobrist(Zobrist *zobrist, Hash hash) {
// 	return hash_find(zobrist->hashtable, hash);
// }

// void *zobrist_insert_state(Zobrist *zobrist, Hash hash, int eval_score, int depth) {
// 	hash_insert(zobrist->hashtable, hash, eval_score, depth);

// }

// get hash from board
// move piece 
// capture piece
// update turn
// check if hash is in the table



