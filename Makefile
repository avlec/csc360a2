CC=gcc
CFLAGS=-Wall -Werror -ansi -std=gnu11

kapish: kapish.c
	$(CC) $(CFLAGS) kapish.c -o kapish
