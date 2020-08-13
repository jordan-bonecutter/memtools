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

test_memtools_disabled: test_memtools_disabled.o libmemtools.a
	$(CC) test_memtools_disabled.o libmemtools.a -o test_memtools_disabled

test_memtools_enabled: test_memtools_enabled.o libmemtools.a
	$(CC) test_memtools_enabled.o libmemtools.a -o test_memtools_enabled

test_memtools_enabled.o: memtools_test.c
	$(CO) -DMEMTOOLS memtools_test.c -o test_memtools_enabled.o

test_memtools_disabled.o: memtools_test.c
	$(CO) memtools_test.c -o test_memtools_disabled.o

libmemtools.a: memtools.o memtools_memory_interface.o
	ar rc libmemtools.a memtools.o memtools_memory_interface.o

memtools.o: memtools.c memtools.h memtools_internal.h
	$(CO) memtools.c -o memtools.o

memtools_memory_interface.o: memtools_memory_interface.h memtools_memory_interface.c
	$(CO) memtools_memory_interface.c -o memtools_memory_interface.o

clean:
	rm -f *.a
	rm -f *.o
	rm -f test_memtools_enabled
	rm -f test_memtools_disabled
	rm -rf *.dSYM

