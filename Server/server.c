#include "includes/chess.h"
#include <string.h>

int serverSocket;
pthread_mutex_t board_lock;
Board *board;

int white_assigned = 0;
int black_assigned = 0;


void sig_handler(int sig) {
    if (sig == SIGINT) {
        close(serverSocket);
        _exit(1);
    } else if (sig == SIGCHLD) {
        int pid;
        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) { }
    }
}

void handle_request(int fd, char *socket_buff, int length, struct thread_args args) {
    char command_char = socket_buff[0];
    if (!((command_char >= '0') && (command_char <= '9'))) {
        printf("%c Received, not a valid command\n", command_char);
        ssize_t w = write(fd, "ERROR", 5);
        (void)w;
        return;
    }

    Command cmd = command_char - '0';

    if (cmd == VIEW) {
        char *fen = board_to_fen(args.chess_board);
        ssize_t w = write(fd, fen, strlen(fen));
        (void)w;
        free(fen);
        return;
    } else if (cmd == GENERATE) {
        if (length < 4) {
            ssize_t w = write(fd, "ERROR", 5);
            (void)w;
            return;
        }
        // printf("buffer: %s\n",socket_buff);
        int index =(7 -(socket_buff[2] - 'a')) + (socket_buff[3] - '1') * 8;
        // printf("INDEX: %d\n",index);
        char *moves = generate_moves_as_string(args.chess_board, index);
        // printf("moves: %s\n",moves);
        ssize_t w = write(fd, moves, strlen(moves));
        (void)w;
        free(moves);
        return;
    } else if (cmd == MOVE) {
        if (length < 7) {
            ssize_t w = write(fd, "ERROR", 5);
            (void)w;
            return;
        }
        // printf("buffer: %s\n",socket_buff);
        int from_index = (7 - (socket_buff[2] - 'a')) + (socket_buff[3] - '1') * 8;
        int to_index = (7 - (socket_buff[5] - 'a')) + (socket_buff[6] - '1') * 8;
        // printf("FROM: %d, TO: %d\n",from_index,to_index);
        Move move = encode_move(from_index, to_index, 0);
        move_piece(args.chess_board, move);
        char *fen = board_to_fen(args.chess_board);
        ssize_t w = write(fd, fen, strlen(fen));
        (void)w;
        free(fen);
        return;
    } else if (cmd == UNDO) {
        ssize_t w = write(fd, "UNDOING", 7);
        (void)w;
        return;
    } else if (cmd == GET_COLOR) {
        char buffer[2];
        buffer[0] = args.color + '0';
        buffer[1] = '\0';
        ssize_t w = write(fd, buffer, 1);
        (void)w;
        return;
    }
    ssize_t w = write(fd, "ERROR", 5);
    (void)w;
    return;
}

void* client_handler(void *void_args) {
    struct thread_args args = *(struct thread_args *)void_args;
    int client_fd = args.client_fd;

    int keep_connection = 1;
    while (keep_connection) {
        char buffer[MAX_BUFFER] = {0};
        char request[MAX_BUFFER] = {0};
        int len = 0;
        char ch;
        // Read until double newline as before.
        while (read(client_fd, &ch, 1)) {
            buffer[len++] = ch;
            if (*request == '\0' && ch == '\n') {
                strncpy(request, buffer, len);
                request[len] = '\0';
            }
            if (len >= 2 && buffer[len - 2] == '\n' && buffer[len - 1] == '\n')
                break;
        }
        if (len == 0) {
            break;
        }
        handle_request(client_fd, buffer, len, args);
    }

    printf("Closing human client socket %d\n", client_fd);
    close(client_fd);
    return NULL;
}


void* viewer_handler(void *void_args) {
    struct thread_args args = *(struct thread_args *)void_args;
    int client_fd = args.client_fd;

    int keep_connection = 1;
    while (keep_connection) {
        char buffer[MAX_BUFFER] = {0};
        int len = 0;
        char ch;
        while (read(client_fd, &ch, 1)) {
            buffer[len++] = ch;
            if (len >= 2 && buffer[len - 2] == '\n' && buffer[len - 1] == '\n')
                break;
        }
        if (len == 0) {
            break;
        }

        Command cmd = buffer[0] - '0';

        if (cmd == VIEW) {
            char *fen = board_to_fen(args.chess_board);
            ssize_t w = write(client_fd, fen, strlen(fen));
            (void)w;
            free(fen);
        } else if (cmd == GET_COLOR) {
        	char color_buff[2];
	        color_buff[0] = '3';
	        color_buff[1] = '\0';
                ssize_t w = write(client_fd, color_buff, 1);
                (void)w;
        } else {
                ssize_t w = write(client_fd, "ERROR", 5);
                (void)w;
        }
    }
    printf("Closing viewer socket %d\n", client_fd);
    close(client_fd);
    return NULL;
}

void* ai_handler(void *void_args) {
    struct thread_args args = *(struct thread_args *)void_args;
    int error;
    while (1) {
        if (is_color_turn(args.chess_board, args.color)) {
            printf("AI (%s) turn\n", (args.color == WHITE ? "WHITE" : "BLACK"));
            Move move = select_move(args.chess_board);
            error = move_piece(args.chess_board, move);
            if (error) {
                printf("error making move\n");
                break;
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s {PORT} {human player count (0, 1, or 2)}\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int human_players = atoi(argv[2]);
    if (human_players < 0 || human_players > 2) {
        fprintf(stderr, "Human player count must be 0, 1, or 2\n");
        exit(1);
    }

    // Initialize board, Zobrist hash and mutex.
    pthread_mutex_init(&board_lock, NULL);
    board = create_board();
    init_zobrist_hash(&zobrist, board);
    printf("board_hash: %llu\n", (unsigned long long)board->z_hash);
    printf("PST\n\tmidgame: %d\n\tendgame: %d\n",board->pst_scores[MIDGAME],board->pst_scores[ENDGAME]);

    // Setup signal handlers.
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    // Setup server address and socket.
    struct sockaddr_in serverIPAddress;
    memset(&serverIPAddress, 0, sizeof(serverIPAddress));
    serverIPAddress.sin_family = AF_INET;
    serverIPAddress.sin_addr.s_addr = INADDR_ANY;
    serverIPAddress.sin_port = htons((uint16_t) port);

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        exit(1);
    }
    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));

    if (bind(serverSocket, (struct sockaddr *) &serverIPAddress, sizeof(serverIPAddress)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(serverSocket, 10) < 0) {
        perror("listen");
        exit(1);
    }

    // For the two player slots, start AI threads immediately if the slot is not intended for a human.
    pthread_t player_threads[2];
    struct thread_args white_args, black_args;

    // White slot: if human is expected, wait for connection; otherwise, spawn AI.
    if (human_players >= 1) {
        printf("Waiting for human player for WHITE slot...\n");
        white_assigned = 0;   // will be set when a human connects.
    } else {
        white_args.color = WHITE;  // assuming WHITE is defined as 1 in your project.
        white_args.client_fd = -1;
        white_args.chess_board = board;
        pthread_create(&player_threads[0], NULL, ai_handler, (void *) &white_args);
        white_assigned = 1;
    }

    // Black slot: if human is expected for two human players, wait; otherwise use AI.
    if (human_players == 2) {
        printf("Waiting for human player for BLACK slot...\n");
        black_assigned = 0;
    } else {
        black_args.color = BLACK;  // assuming BLACK is defined as 0.
        black_args.client_fd = -1;
        black_args.chess_board = board;
        pthread_create(&player_threads[1], NULL, ai_handler, (void *) &black_args);
        black_assigned = 1;
    }

    // Main loop: accept incoming connections.
    // If a designated human player slot is available, assign the connection to that slot;
    // otherwise, treat the connection as a viewer.
    struct sockaddr_in clientIPAddress;
    socklen_t alen = sizeof(clientIPAddress);
    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientIPAddress, &alen);
        if (clientSocket < 0) {
            perror("accept");
            continue;
        }
        struct thread_args args;
        args.client_fd = clientSocket;
        args.chess_board = board;

        // If white slot is not yet filled and we expect a human player (always white),
        // assign this connection as the white human player.
        if (!white_assigned && human_players >= 1) {
            args.color = WHITE;
            pthread_t t;
            if (pthread_create(&t, NULL, client_handler, (void *) &args) != 0) {
                perror("pthread_create (white client_handler)");
                close(clientSocket);
            } else {
                // Detach or join later as needed.
                pthread_detach(t);
                white_assigned = 1;
                printf("Human player assigned to WHITE slot.\n");
            }
        }
        // Else if black slot is available (only when human_players is 2)
        else if (!black_assigned && human_players == 2) {
            args.color = BLACK;
            pthread_t t;
            if (pthread_create(&t, NULL, client_handler, (void *) &args) != 0) {
                perror("pthread_create (black client_handler)");
                close(clientSocket);
            } else {
                pthread_detach(t);
                black_assigned = 1;
                printf("Human player assigned to BLACK slot.\n");
            }
        }
        // Otherwise, treat this connection as a viewer.
        else {
            // For viewers, we only allow VIEW commands.
            args.color = -1;  // color not used by viewer_handler.
            pthread_t t;
            if (pthread_create(&t, NULL, viewer_handler, (void *) &args) != 0) {
                perror("pthread_create (viewer_handler)");
                close(clientSocket);
            } else {
                // Detach viewer threads – we don’t need to join these later.
                pthread_detach(t);
                printf("Viewer connected.\n");
            }
        }
    }

    close(serverSocket);
    return 0;
}
