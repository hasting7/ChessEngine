#ifndef STRUCTS_H
#define STRUCTS_H

extern pthread_rwlock_t board_mutex;
extern pthread_rwlock_t zobrist_rwlock;


typedef enum color_enum {
    WHITE = 0,
    BLACK
} Color;

typedef enum pieces_enum {
    PAWN = 0,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    NO_PIECE
} Piece;

typedef enum command_enum {
    MOVE = 0,
    VIEW,
    UNDO,
    GENERATE,
    GET_COLOR,
} Command;

typedef uint64_t Hash;
typedef uint64_t Bitboard;

#define BOARD_SIZE 64
#define NUM_PIECES 6
#define NUM_COLORS 2 

typedef struct board_struct {
    Bitboard pieces[2][6];
    Bitboard all_pieces[2];
    int move_count;
    int last_capture;
    Color active_player;
    // add something for En passant rule
    // add somthing for castle rule
} Board;

typedef struct hash_entry {
    Hash hash;
    int eval_score;
    int depth;
    struct hash_entry *next;
} HashEntry;

typedef struct hashtable_struct {
    int size;
    struct hash_entry **table;
} Hashtable;

typedef struct zobrist_struct {
    Hash board[NUM_COLORS][NUM_PIECES][BOARD_SIZE];
    Hash white_turn_hash;
    Hash black_turn_hash;
    Hash hash;
    Hashtable *hashtable;
    // uint64_t is_black_turn;
    // add en passant later
    // add castling later
    // add promotion later
} Zobrist;

struct thread_args {
    int client_fd;
    Board *chess_board;
    Zobrist *zobrist;
    Color color;
};

#endif // STRUCTS_H
