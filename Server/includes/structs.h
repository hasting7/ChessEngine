#ifndef STRUCTS_H
#define STRUCTS_H

typedef enum { 
    EXACT, 
    LOWER, 
    UPPER
} HashFlag;

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

typedef enum flag_name_enum {
    WHITE_CHECK = 0,
    BLACK_CHECK,
    TERMINAL
} BoardFlag;


typedef uint64_t Hash;
typedef uint64_t Bitboard;
typedef uint16_t Move;

#define NULL_MOVE (0);

#define BOARD_SIZE 64
#define NUM_PIECES 6
#define NUM_COLORS 2 
#define TRUE    1
#define FALSE   0

typedef struct board_struct {
    Bitboard pieces[2][6];
    Bitboard all_pieces[2];
    Bitboard attackable[2];
    uint8_t piece_count[2][6];
    Hash z_hash;
    int move_count;
    int last_capture;
    Color active_player;
    uint8_t flags;          // 00000000 {terminal state, white check, black check, ....}

    // char can_castle[2][2];  // [player][kingsize, queensize]
    // add something for En passant rule
    // add somthing for castle rule
} Board;



struct board_data {
    int eval_score;
    int depth;
    HashFlag flags;
    Move best_move;
};

typedef struct hash_entry {
    Hash hash;
    struct board_data data;
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
    Hashtable *hashtable;
    // uint64_t is_black_turn;
    // add en passant later
    // add castling later
    // add promotion later
} Zobrist;

struct thread_args {
    int client_fd;
    Board *chess_board;
    Color color;
};

struct alphabeta_response {
    Move move;
    float score;
};

#endif // STRUCTS_H
