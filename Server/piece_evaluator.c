#include "includes/chess.h"

#define PHASE_PAWN     0
#define PHASE_KNIGHT   1
#define PHASE_BISHOP   1
#define PHASE_ROOK     2
#define PHASE_QUEEN    4
#define PST_VAL(piece, phase, sq) ((*piece_psts[(piece)])[(phase)][(sq)])

int rook_bonus(Board *);
int pawn_bonus(Board *);

int evaluate_pieces(Board *state) {
	int phase = compute_phase(state);

	int score = 0;
	// count up pieces
	score += calculate_piece_advantange(QUEEN, state->piece_count[WHITE][QUEEN] - state->piece_count[BLACK][QUEEN], phase);
	score += calculate_piece_advantange(ROOK, state->piece_count[WHITE][ROOK] - state->piece_count[BLACK][ROOK], phase);
	score += calculate_piece_advantange(BISHOP, state->piece_count[WHITE][BISHOP] - state->piece_count[BLACK][BISHOP], phase);
	score += calculate_piece_advantange(KNIGHT, state->piece_count[WHITE][KNIGHT] - state->piece_count[BLACK][KNIGHT], phase);
	score += calculate_piece_advantange(PAWN, state->piece_count[WHITE][PAWN] - state->piece_count[BLACK][PAWN], phase);

	// printf("Phase: %d\nPiece count adv: %d\nPST adv: %d\n",phase,score,(state->pst_scores[MIDGAME] * phase + state->pst_scores[ENDGAME] * (256 - phase)) / 256);

	score += (state->pst_scores[MIDGAME] * phase + state->pst_scores[ENDGAME] * (256 - phase)) / 256;
	// use pst

	return score;
}

int rook_bonus(Board *state) {
	// Bitboard rank = 0x00000000000000FF;
	// Bitboard file = 0x0101010101010101;

	// files better than row
	// my color & rook & file

	// open file / rank
	return 0;
}

int pawn_bonus(Board *state) {
	return 0;
}

int calculate_piece_advantange(Piece piece, int piece_delta, int phase) {
	int mg_score = piece_values[MIDGAME][piece];
	int eg_score = piece_values[ENDGAME][piece];
	// piece_delta = white - black
	return ((mg_score * phase + eg_score * (256 - phase)) / 256) * piece_delta;
}

void initialize_pst(Board *state) {
	Bitboard tile_mask = 1ULL;
	Piece piece_type;
	for (int i = 0; i < 64; i++) {
		if (state->all_pieces[WHITE] & tile_mask) {
			piece_type = piece_on_tile(state, WHITE, tile_mask);
			state->pst_scores[MIDGAME] += PST_VAL(piece_type, MIDGAME, i);
			state->pst_scores[ENDGAME] += PST_VAL(piece_type, ENDGAME, i);

		} else if (state->all_pieces[BLACK] & tile_mask) {
			piece_type = piece_on_tile(state, BLACK, tile_mask);
			state->pst_scores[MIDGAME] -= PST_VAL(piece_type, MIDGAME, 63 - i);
			state->pst_scores[ENDGAME] -= PST_VAL(piece_type, ENDGAME, 63 - i);
		}
		tile_mask <<= 1;
	}
}

void pst_remove_piece(Board *state, Color color, Piece type, int from_tile) {
	int sign = 1;
	if (color == BLACK) {
		from_tile = 63 - from_tile;
		sign = -1;
	}
	state->pst_scores[MIDGAME] -= sign * PST_VAL(type, MIDGAME, from_tile);
	state->pst_scores[ENDGAME] -= sign * PST_VAL(type, ENDGAME, from_tile);
}

void pst_add_piece(Board *state, Color color, Piece type, int to_tile) {
	int sign = 1;
	if (color == BLACK) {
		to_tile = 63 - to_tile;
		sign = -1;
	}
	state->pst_scores[MIDGAME] += sign * PST_VAL(type, MIDGAME, to_tile);
	state->pst_scores[ENDGAME] += sign * PST_VAL(type, ENDGAME, to_tile);
}

int compute_phase(Board *state) {
    int phase = MAX_PHASE;

    phase -= state->piece_count[WHITE][KNIGHT] * PHASE_KNIGHT;
    phase -= state->piece_count[WHITE][BISHOP] * PHASE_BISHOP;
    phase -= state->piece_count[WHITE][ROOK]   * PHASE_ROOK;
    phase -= state->piece_count[WHITE][QUEEN]  * PHASE_QUEEN;

    phase -= state->piece_count[BLACK][KNIGHT] * PHASE_KNIGHT;
    phase -= state->piece_count[BLACK][BISHOP] * PHASE_BISHOP;
    phase -= state->piece_count[BLACK][ROOK]   * PHASE_ROOK;
    phase -= state->piece_count[BLACK][QUEEN]  * PHASE_QUEEN;

    // scale to 0–256
    int scaled = (phase * 256) / MAX_PHASE;
    return scaled; // 0 = endgame, 256 = opening
}



const int MAX_PHASE = 
    PHASE_KNIGHT * 4 +  // 2 knights per side
    PHASE_BISHOP * 4 +  // 2 bishops
    PHASE_ROOK * 4 +  // 2 rooks
    PHASE_QUEEN * 2;   // 1 queen per side
// = 24


const int piece_values[2][6] = {
    {
    	100, 500, 320, 330, 1000, 0
    }, // pawn, rook, knight, bishop, queen, king
    {
    	130, 550, 300, 350, 1000, 0
    }
};


const int pawn_pst[2][64] = {
     
    {
    	0,   0,   0,   0,   0,   0,   0,   0,
   		10,  10,  10, -10, -10,  10,  10,  10,
     	5,   5,  10,  20,  20,  10,   5,   5,
     	0,   0,   5,  20,  20,   5,   0,   0,
     	5,   5,  10,  20,  20,  10,   5,   5,
    	10,  10,  20,  30,  30,  20,  10,  10,
    	50,  50,  50,  50,  50,  50,  50,  50,
     	0,   0,   0,   0,   0,   0,   0,   0
    },
    {
    	0,   0,   0,   0,   0,   0,   0,   0,
   		10,  10,  10, -10, -10,  10,  10,  10,
     	5,   5,  10,  20,  20,  10,   5,   5,
     	0,   0,   5,  20,  20,   5,   0,   0,
     	5,   5,  10,  20,  20,  10,   5,   5,
    	10,  10,  20,  30,  30,  20,  10,  10,
    	50,  50,  50,  50,  50,  50,  50,  50,
     	0,   0,   0,   0,   0,   0,   0,   0
    }
};

const int knight_pst[2][64] = {
	{
		-50, -40, -30, -30, -30, -30, -40, -50,
	    -40, -20,   0,   5,   5,   0, -20, -40,
	    -30,   5,  10,  15,  15,  10,   5, -30,
	    -30,   0,  15,  20,  20,  15,   0, -30,
	    -30,   5,  15,  20,  20,  15,   5, -30,
	    -30,   0,  10,  15,  15,  10,   0, -30,
	    -40, -20,   0,   0,   0,   0, -20, -40,
	    -50, -40, -30, -30, -30, -30, -40, -50
	},
	{
		-30, -20, -10, -10, -10, -10, -20, -30,
	    -20,  -5,   0,   5,   5,   0,  -5, -20,
	    -10,   0,  10,  10,  10,  10,   0, -10,
	    -10,   5,  10,  15,  15,  10,   5, -10,
	    -10,   0,  15,  20,  20,  15,   0, -10,
	    -10,   5,  10,  15,  15,  10,   5, -10,
	    -20,  -5,   0,   0,   0,   0,  -5, -20,
	    -30, -20, -10, -10, -10, -10, -20, -30
	}
};

const int bishop_pst[2][64] = {
	{
		-20, -10, -10, -10, -10, -10, -10, -20,
	    -10,   0,   0,   0,   0,   0,   0, -10,
	    -10,   0,   5,  10,  10,   5,   0, -10,
	    -10,   5,   5,  10,  10,   5,   5, -10,
	    -10,   0,  10,  10,  10,  10,   0, -10,
	    -10,  10,  10,  10,  10,  10,  10, -10,
	    -10,   5,   0,   0,   0,   0,   5, -10,
	    -20, -10, -10, -10, -10, -10, -10, -20
	},
	{
		-10, -10, -10, -10, -10, -10, -10, -10,
	   	-10,   5,   0,   0,   0,   0,   5, -10,
	    -10,   0,  10,  10,  10,  10,   0, -10,
	    -10,   0,  10,  15,  15,  10,   0, -10,
	    -10,   0,  15,  20,  20,  15,   0, -10,
	    -10,   0,  10,  15,  15,  10,   0, -10,
	    -10,   5,   0,   0,   0,   0,   5, -10,
	    -10, -10, -10, -10, -10, -10, -10, -10
	}
};

const int rook_pst[2][64] = {
	{
		0,   0,   5,  10,  10,   5,   0,   0,
	    -5,   0,   0,   0,   0,   0,   0,  -5,
	    -5,   0,   0,   0,   0,   0,   0,  -5,
	    -5,   0,   0,   0,   0,   0,   0,  -5,
	    -5,   0,   0,   0,   0,   0,   0,  -5,
	    -5,   0,   0,   0,   0,   0,   0,  -5,
	     5,  10,  10,  10,  10,  10,  10,   5,
	     0,   0,   0,   0,   0,   0,   0,   0
	},
	{
		 0,   0,   5,  10,  10,   5,   0,   0,
	     0,   0,   0,   0,   0,   0,   0,   0,
	     0,   0,   0,   0,   0,   0,   0,   0,
	     5,   5,   5,   5,   5,   5,   5,   5,
	     5,  10,  10,  10,  10,  10,  10,   5,
	    10,  10,  10,  10,  10,  10,  10,  10,
	    10,  10,  10,  10,  10,  10,  10,  10,
	     0,   0,   0,   0,   0,   0,   0,   0
	}
};

const int queen_pst[2][64] = {
	{
		-20, -10, -10,  -5,  -5, -10, -10, -20,
	    -10,   0,   0,   0,   0,   0,   0, -10,
	    -10,   0,   5,   5,   5,   5,   0, -10,
	    -5,   0,   5,   5,   5,   5,   0,  -5,
	     0,   0,   5,   5,   5,   5,   0,  -5,
	    -10,   5,   5,   5,   5,   5,   0, -10,
	    -10,   0,   5,   0,   0,   0,   0, -10,
	    -20, -10, -10,  -5,  -5, -10, -10, -20
	},
	{
		-20, -10, -10,  -5,  -5, -10, -10, -20,
	    -10,   0,   0,   0,   0,   0,   0, -10,
	    -10,   0,   5,   5,   5,   5,   0, -10,
	    -5,   0,   5,   5,   5,   5,   0,  -5,
	     0,   0,   5,   5,   5,   5,   0,  -5,
	    -10,   5,   5,   5,   5,   5,   0, -10,
	    -10,   0,   5,   0,   0,   0,   0, -10,
	    -20, -10, -10,  -5,  -5, -10, -10, -20
	}
};

const int king_pst[2][64] = {
    {
        20,  30,  10,   0,   0,  10,  30,  20,
        20,  20,   0,   0,   0,   0,  20,  20,
       -10, -20, -20, -20, -20, -20, -20, -10,
       -20, -30, -30, -40, -40, -30, -30, -20,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30
    },
    {
       -50, -30, -30, -30, -30, -30, -30, -50,
       -30, -30,   0,   0,   0,   0, -30, -30,
       -30, -10,  20,  30,  30,  20, -10, -30,
       -30, -10,  30,  40,  40,  30, -10, -30,
       -30, -10,  30,  40,  40,  30, -10, -30,
       -30, -10,  20,  30,  30,  20, -10, -30,
       -30, -20, -10,   0,   0, -10, -20, -30,
       -50, -40, -30, -20, -20, -30, -40, -50
    }
};

const PST* piece_psts[6] = {
    &pawn_pst,
    &rook_pst,
    &knight_pst,
    &bishop_pst,
    &queen_pst,
    &king_pst
};