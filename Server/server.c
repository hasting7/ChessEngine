#include "includes/chess.h"

int player_count = 0;
Board *board;
Zobrist zobrist;
pthread_t client_threads[2];


void sig_handler(int sig) {
	if (sig == SIGINT) {
		close( serverSocket );
		_exit(1);
	} else if ( sig == SIGCHLD) {
		int pid;
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {};
	}
}

void handle_request(int fd, char *socket_buff, int length, struct thread_args args) {
	char command_char = socket_buff[0];
	if (!((command_char >= 48) && (command_char <= 57))) {
		printf("%c Recived, not a valid command\n", command_char);
		write(fd, "ERROR", 5);
		return;
	}

	Command cmd = command_char - 48;

	if (cmd == VIEW) {
		char *fen = board_to_fen(args.chess_board);
		write(fd, fen, strlen(fen));
		free(fen);
		return;
	} else if (cmd == GENERATE) {
		if (length < 4) {
			// not enough args
			write(fd, "ERROR", 5);
			return;
		}
		// should be '_ a1'
		// so rank is index 2
		// files is index 3
		int index = (socket_buff[2] - 97) * 8 + (socket_buff[3] - 49);
		char *moves = generate_moves_as_string(args.chess_board,index);
		write(fd, moves, strlen(moves));
		free(moves);
		return;
	} else if (cmd == MOVE) {
		if (length < 7) {
			// not enough args
			write(fd, "ERROR", 5);
			return;
		}
		// should be '_ a1 b2'
		int from_index = (socket_buff[2] - 97) * 8 + (socket_buff[3] - 49);
		int to_index = (socket_buff[5] - 97) * 8 + (socket_buff[6] - 49);
		uint64_t moves = generate_moves(args.chess_board, from_index);

		int status = move_piece(args.chess_board, args.zobrist, from_index, to_index, moves, args.color);
		printf("resp status on moving %d\n", status);

		char *fen = board_to_fen(args.chess_board);
		write(fd, fen, strlen(fen));
		free(fen);
		return;
	} else if (cmd == UNDO) {
		write(fd, "UNDOING", 7);
		return;
	} else if (cmd == GET_COLOR){
		char buffer[2];
		buffer[0] = args.color + 48;
		buffer[1] = '\0';
		write(fd, buffer, 1);
		return;
	}
	write(fd, "ERROR", 5);
	return;
}

void* client_handler(void *void_args) {
	struct thread_args args = *(struct thread_args *) void_args;
	int client_fd = args.client_fd;

	int keep_connection = 1;

	while (keep_connection) {
		char buffer[MAX_BUFFER] = { 0 };
		char request[MAX_BUFFER] = { 0 };
		int len = 0 ;
		char ch;
		while ( read(client_fd, &ch, 1)) {
			buffer[len++] = ch;
			if (*request == '\0' && ch == '\n') {
				strncpy(request, buffer, len);
				request[len] = '\0';
			}
			if (
				(len >= 2) &&
				(buffer[len - 2] == '\n') &&
				(buffer[len - 1] == '\n')
			) break;
		}
		if (len == 0) { // empty message socket closed
			break;
		}
		printf("Bytes Recived: %d\n",len);
		buffer[len - 2] = '\0';
		printf("Message: %s\n",buffer);

		handle_request(client_fd, buffer, len, args);
	}

	printf("Closing %d\n",client_fd);

	close(client_fd);
	return NULL;
}

int main(int argc, char **argv) {

    // Create the board and initialize Zobrist hash
    board = create_board();
    init_zobrist_hash(&zobrist, board);
    printf("board_hash: %llu\n", zobrist.hash);

    int port = PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc >= 3) {
        fprintf(stderr, "Usage: ./server {PORT}\n");
        exit(1);
    }

    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGINT, &act, NULL);

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

    int optval = -1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));

    int err = bind(serverSocket, (struct sockaddr *) &serverIPAddress, sizeof(serverIPAddress));
    if (err) {
        perror("bind");
        exit(1);
    }

    err = listen(serverSocket, 2);
    if (err) {
        perror("listen");
        exit(1);
    }

    struct sockaddr_in clientIPAddress;
    int alen = sizeof(clientIPAddress);
    int clientSocket;
    
    while (1) {
        if (player_count < 2) {
            clientSocket = accept(serverSocket, (struct sockaddr *) &clientIPAddress, (socklen_t*) &alen);
            if (clientSocket < 0) {
                perror("accept");
                continue;
            }

            struct thread_args args = { .color = player_count, .client_fd = clientSocket, .chess_board = board, .zobrist = &zobrist };

            player_count++;

            if (pthread_create(&client_threads[player_count - 1], NULL, client_handler, (void *) &args) != 0) {
                perror("pthread_create");
                player_count--;
            }
        } else {
            printf("Server is full. Waiting for a player to disconnect...\n");


            clientSocket = accept(serverSocket, (struct sockaddr *) &clientIPAddress, (socklen_t*) &alen);
            if (clientSocket > 0) {
                close(clientSocket);
            }
        }


        if (player_count == 2) {
            pthread_join(client_threads[0], NULL);
            pthread_join(client_threads[1], NULL);
            player_count = 0;
        }
    }

    close(serverSocket);
    return 0;
}