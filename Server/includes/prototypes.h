#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "structs.h"
// BOARD PROTOTYPES

void reset_board(Board **);
Board * create_board();
char *board_to_fen(Board *);
Piece piece_on_tile(Board *, Color, Bitboard);
Bitboard generate_moves(Board *, int, int);
char *generate_moves_as_string(Board *, int);
Bitboard generate_knight_moves(Board *, Bitboard, Color);
Bitboard generate_pawn_moves(Board *, Bitboard, Color);
Bitboard generate_rook_moves(Board *, Bitboard, Color);
Bitboard generate_bishop_moves(Board *, Bitboard, Color);
Bitboard generate_queen_moves(Board *, Bitboard, Color); 
Bitboard generate_king_moves(Board *, Bitboard, Color);
int move_piece(Board *, Move);
void pawn_promote(Board *, int);

int is_color_turn(Board *board, Color color);

Move encode_move(int from_index, int to_index, int flags);
void decode_move(Move move, int *from_index, int *to_index, int *flags);
int check_move_flag(Move);
Bitboard generate_attackable_squares(Board *, Color);

int check_flag(Board *, BoardFlag);
void set_flag(Board *, BoardFlag, int);
void load_magic_file(const char *filename, MagicData *table);
void init_masks();
Bitboard bmask(int);
Bitboard rmask(int);
int magic_index(Bitboard, Bitboard, Bitboard, int); 

extern MagicData rook_magic[64];
extern MagicData bishop_magic[64];
extern Bitboard rook_masks[64];
extern Bitboard bishop_masks[64];

// HASH PROTOTYPES

Hashtable *create_hashtable(int);
void delete_hashtable(Hashtable *);
void hash_insert(Hashtable *, Hash, void *);
int hash_remove(Hashtable *, Hash);
void *hash_find(Hashtable *, Hash);
void hash_clear(Hashtable *table);
Hash rnd64();

// SERVER PROTOTYPES

void sig_handler(int);
extern Zobrist zobrist;
extern Board *board;
extern int seed;
extern int player_count;
extern pthread_t client_threads[2];

// ZOBRIST PROTOTYPES


void init_zobrist_hash(Zobrist *, Board *);
Hash update_zobrist_move( int, int, Piece, Color); 
Hash get_board_hash(Board *);
Hash update_zobrist_capture(int, Piece, Color);
Hash update_zobrist_turn();
Hash update_pawn_promote(int, Piece, Color);
// AI PROTOTYPES

Move select_move(Board *);
void alphabeta(Board *, int, int, int, int, struct alphabeta_response *, Bitboard);


// PIECE EVALUATOR

void pst_remove_piece(Board *, Color, Piece, int);
void pst_add_piece(Board *, Color, Piece, int);
int evaluate_pieces(Board *);
int compute_phase(Board *);
int calculate_piece_advantange(Piece, int, int);
void initialize_pst(Board *); 

extern const PST pawn_pst;
extern const PST knight_pst;
extern const PST rook_pst;
extern const PST bishop_pst;
extern const PST queen_pst;
extern const PST king_pst;
extern const int piece_values[2][6];
extern const PST* piece_psts[6];
extern const int MAX_PHASE;

// MAGIC BITBOARD

void create_magic_file(const char *filename, int is_rook);
void load_magic_file(const char *filename, MagicData *table);
void init_masks();
Bitboard rmask(int);
Bitboard bmask(int);
Bitboard ratt(int, Bitboard);
Bitboard batt(int, Bitboard);


#endif
