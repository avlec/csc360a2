CC=gcc
CFLAGS=-Wall -Werror -ansi -std=gnu11

kapish: src/kapish.c
	$(CC) $(CFLAGS) src/kapish.c -o kapish
