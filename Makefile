CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Targets
all: test_static test_dynamic

# Static version
test_static: allocator_static.o tests.o
	$(CC) $(CFLAGS) allocator_static.o tests.o -o test_static

# Dynamic version
test_dynamic: allocator_dynamic.o tests.o
	$(CC) $(CFLAGS) allocator_dynamic.o tests.o -o test_dynamic

# Compile source files
allocator_static.o: allocator_static.c allocator.h
	$(CC) $(CFLAGS) -c allocator_static.c

allocator_dynamic.o: allocator_dynamic.c allocator.h
	$(CC) $(CFLAGS) -c allocator_dynamic.c

tests.o: tests.c allocator.h
	$(CC) $(CFLAGS) -c tests.c

# Clean
clean:
	rm -f *.o test_static test_dynamic

.PHONY: all clean