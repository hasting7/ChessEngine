#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include "structs.h"
// BOARD PROTOTYPES

Board * create_board();
char *board_to_fen(Board *);
Piece piece_on_tile(Board *, Color, uint64_t);
uint64_t generate_moves(Board *, int);
char *generate_moves_as_string(Board *, int);
uint64_t generate_knight_moves(Board *, uint64_t, Color);
uint64_t generate_pawn_moves(Board *, uint64_t, Color);
uint64_t generate_rook_moves(Board *, uint64_t, Color);
uint64_t generate_bishop_moves(Board *, uint64_t, Color);
uint64_t generate_queen_moves(Board *, uint64_t, Color); 
uint64_t generate_king_moves(Board *, uint64_t, Color);
int move_piece(Board *, Zobrist *, int, int, Bitboard, Color);


// HASH PROTOTYPES

Hashtable *create_hashtable(int);
void delete_hashtable(Hashtable *);
void hash_insert(Hashtable *, Hash, int, int);
int hash_remove(Hashtable *, Hash);
HashEntry * hash_find(Hashtable *, Hash);

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
void update_zobrist_move(Zobrist *, int, int, Piece, Color); 
Hash get_board_hash(Zobrist *, Board *);
void update_zobrist_capture(Zobrist *, int, Piece, Color);
void update_zobrist_turn(Zobrist *);


#endif