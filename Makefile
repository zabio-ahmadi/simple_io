
CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -g -O3 -fsanitize=leak -fsanitize=address
# -g : Flag for implicit rules. Turn on debug info
# -03 : optimisation level 3 

.phony=ultra-cp # force to compile ultra-cp
ultra-cp: ultra-cp.c
	gcc  $(CFLAGS) $^ -o $@ 

# $^ ultra-cp.c (every objectif that it finds)
# $@ == ultra-cp  
run:
	./ultra-cp

clean:
	rm -rf *.o ultra-cp

rebuild: clean compile