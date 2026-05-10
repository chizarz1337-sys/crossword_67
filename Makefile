CC     = gcc
CFLAGS = -I include -I files -Wall -g

COMMON = src/censAVLtree.c src/dictionary.c src/logger.c \
         files/grid.c files/generator.c files/display.c

all:
	$(CC) $(CFLAGS) src/main.c $(COMMON) -o crossword_gen

benchmark:
	$(CC) $(CFLAGS) tests/benchmark.c $(COMMON) -lm -o tests/benchmark

clean:
	rm -f crossword_gen tests/benchmark crossword.log *.bin
	rm -f tests/results/*.csv tests/results/*.bin

.PHONY: all benchmark clean
