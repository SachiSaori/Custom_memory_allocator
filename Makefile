CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Targets
all: test_program

# Link object files into executable
test_program: allocator_static.o tests.o
	$(CC) $(CFLAGS) allocator_static.o tests.o -o test_program

# Compile source files to object files
allocator_static.o: allocator_static.c allocator.h
	$(CC) $(CFLAGS) -c allocator_static.c

tests.o: tests.c allocator.h
	$(CC) $(CFLAGS) -c tests.c

# Clean up compiled files
clean:
	rm -f *.o test_program

# Phony targets (not actual files)
.PHONY: all clean