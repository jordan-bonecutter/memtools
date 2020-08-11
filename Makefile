#
#
#
#

COMPILER = gcc
DEBUG    = -g -O0
FLAGS    = -ansi -std=c99 -Wall $(DEBUG)
CC			 = $(COMPILER) $(FLAGS)
CO       = $(CC) -c

test: test.o memtools.o
	$(CC) test.o memtools.o -o test

test.o: memtools_test.c
	$(CO) -DMEMTOOLS memtools_test.c -o test.o

memtools.o: memtools.c
	$(CO) memtools.c -o memtools.o

clean:
	rm -f *.o
	rm -f test
	rm -rf *.dSYM

