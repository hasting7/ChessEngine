#ifndef _BOARD_H_
#define _BOARD_H_


#define WHITE_PAWN   ('P')
#define BLACK_PAWN   ('p')

#define WHITE_ROOK   ('R')
#define BLACK_ROOK   ('r')

#define WHITE_KNIGHT ('N')
#define BLACK_KNIGHT ('n')

#define WHITE_BISHOP ('B')
#define BLACK_BISHOP ('b')

#define WHITE_QUEEN  ('Q')
#define BLACK_QUEEN  ('q')

#define WHITE_KING   ('K')
#define BLACK_KING   ('k')

#define WHITE_STR ('w')
#define BLACK_STR ('b')

#define MAX_MOVES (28)
#define NO_MOVE (0)

#define WHITE_PAWN_START   (0x000000000000FF00) 
#define WHITE_ROOK_START   (0x0000000000000081)  
#define WHITE_KNIGHT_START (0x0000000000000042) 
#define WHITE_BISHOP_START (0x0000000000000024)
#define WHITE_KING_START   (0x0000000000000010)
#define WHITE_QUEEN_START  (0x0000000000000008)

#define BLACK_PAWN_START   (0x00FF000000000000) 
#define BLACK_ROOK_START   (0x8100000000000000) 
#define BLACK_KNIGHT_START (0x4200000000000000)
#define BLACK_BISHOP_START (0x2400000000000000) 
#define BLACK_KING_START   (0x1000000000000000) 
#define BLACK_QUEEN_START  (0x0800000000000000)
// White pieces placed near the center, spaced out
// #define WHITE_PAWN_START      (0x0000000000100000)  // White pawn on d4 (square 35)
// #define WHITE_ROOK_START      (0x0000000000001000)  // White rook on e3 (square 44)
// #define WHITE_KNIGHT_START    (0x0000000000000400)  // White knight on c3 (square 50)
// #define WHITE_BISHOP_START    (0x0000000000000020)  // White bishop on f2 (square 53)
// #define WHITE_KING_START      (0x0000000000000008)  // White king on d2 (square 51)
// #define WHITE_QUEEN_START     (0x0000000000000004)  // White queen on e5 (square 28)

// // Black pieces placed near the center, spaced out
// #define BLACK_PAWN_START      (0x0000001000000000)  // Black pawn on e5 (square 28)
// #define BLACK_ROOK_START      (0x0000100000000000)  // Black rook on d6 (square 19)
// #define BLACK_KNIGHT_START    (0x0000040000000000)  // Black knight on c6 (square 18)
// #define BLACK_BISHOP_START    (0x0000002000000000)  // Black bishop on f7 (square 13)
// #define BLACK_KING_START      (0x0000000800000000)  // Black king on d7 (square 12)
// #define BLACK_QUEEN_START     (0x0000000400000000)  // Black queen on e4 (square 36)


#define NOT_A_FILE  (0xfefefefefefefefe)
#define NOT_H_FILE  (0x7f7f7f7f7f7f7f7f)


#endif