CC=gcc
CFLAGS=-Wall -Werror -Wextra -std=c11 -g
all: buggy fixed
buggy: buggy.c
	$(CC) $(CFLAGS) buggy.c -o buggy
fixed: fixed.c
	$(CC) $(CFLAGS) fixed.c -o fixed
run: fixed
	./fixed amy
test: fixed
	$(CC) $(CFLAGS) -DFIXED_TEST tests/test_main.c fixed.c -o test_main
	./test_main
clean:
	rm -f buggy fixed test_main
