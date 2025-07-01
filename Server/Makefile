CC = cc

# Compiler and linker flags
CFLAGS = -Wall -Werror -I./includes -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG -pthread
LDFLAGS = -flto -O3 -march=native -pthread

# Source files
SRCS = server.c board.c hash.c zobrist_hash.c chess_agent.c piece_evaluator.c thread_pool.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = server

# Default target to build
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
