CC = cc
CFLAGS = -Wall -Werror -I./includes

# Source files
SRCS = server.c board.c hash.c zobrist_hash.c chess_agent.c
# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = server

# Default target to build
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -O3

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
