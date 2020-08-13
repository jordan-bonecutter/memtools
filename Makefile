#
#
#
#

COMPILER = gcc
DEBUG    = -O0 -g
FAST     = -O3
FLAGS    = -ansi -std=c99 -Wall $(DEBUG)
CC			 = $(COMPILER) $(FLAGS)
CO       = $(CC) -c

all: test_memtools_disabled test_memtools_enabled

test_memtools_disabled: test_memtools_disabled.o memtools.o
	$(CC) test_memtools_disabled.o memtools.o -o test_memtools_disabled

test_memtools_enabled: test_memtools_enabled.o memtools.o
	$(CC) test_memtools_enabled.o memtools.o -o test_memtools_enabled

test_memtools_enabled.o: memtools_test.c
	$(CO) -DMEMTOOLS memtools_test.c -o test_memtools_enabled.o

test_memtools_disabled.o:
	$(CO) memtools_test.c -o test_memtools_disabled.o

memtools.o: memtools.c memtools.h memtools_internal.h
	$(CO) memtools.c -o memtools.o

clean:
	rm -f *.o
	rm -f test_memtools_enabled
	rm -f test_memtools_disabled
	rm -rf *.dSYM

