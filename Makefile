# Compiler and flags
CC = gcc
CFLAGS = -pthread -Wall

# source files
SRCS = server.c 
EXECUTABLE = assignment3

#object files 
OBJS = $(SRCS:.c=.o)

# Default target (all)
all: $(EXECUTABLE)

# Compile each .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into the executable
$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Clean up the project (remove object files and executable)
clean:
	rm -f $(OBJS) $(EXECUTABLE)