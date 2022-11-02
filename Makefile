
CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -DEFAULT_SOURCE  -g -O3 -fsanitize=leak -fsanitize=address

# all:ultra-cp
# ultra-cp:
# 	$(CC) -c main.c $(CFLAGS) -o ultra-cp


all:
	gcc main.c $(CFLAGS) -o ultra-cp
run:
	./ultra-cp

clean:
	rm -rf *.o ultra-cp

rebuild: clean compile