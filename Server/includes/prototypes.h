#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "structs.h"
// BOARD PROTOTYPES

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
int check_move_flag(Move, int);
Bitboard generate_attackable_squares(Board *, Color);

int check_flag(Board *, BoardFlag);
void set_flag(Board *, BoardFlag, int);

extern const int CAPTURE;
extern const int NORMAL;


// HASH PROTOTYPES

Hashtable *create_hashtable(int);
void delete_hashtable(Hashtable *);
void hash_insert(Hashtable *, Hash, struct board_data);
int hash_remove(Hashtable *, Hash);
struct board_data * hash_find(Hashtable *, Hash);

// SERVER PROTOTYPES

void sig_handler(int);
extern Zobrist zobrist;
extern Board *board;
extern int seed;
extern int player_count;
extern pthread_t client_threads[2];

// ZOBRIST PROTOTYPES

uint64_t rnd64();
void init_zobrist_hash(Zobrist *, Board *);
Hash update_zobrist_move( int, int, Piece, Color); 
Hash get_board_hash(Board *);
Hash update_zobrist_capture(int, Piece, Color);
Hash update_zobrist_turn();
Hash update_pawn_promote(int, Piece, Color);
// AI PROTOTYPES

Move select_move(Board *);
void alphabeta(Board *, int, int, int, int, struct alphabeta_response *, Bitboard);
void process_task(Board, int, Bitboard);

#endif