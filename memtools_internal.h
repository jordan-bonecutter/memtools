/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools_internal.h * * * * * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MEMTOOLS_INTERNAL_INCLUDE_GUARD
#define MEMTOOLS_INTERNAL_INCLUDE_GUARD

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void* memtools_malloc (size_t n, unsigned int line, char* file);/* Version of malloc which keeps track of line & file where memory was allocated */
void  memtools_free(void* ptr, unsigned int line, char* file); /* Version of free which keeps track of line and file where memory was deallocated */
void* memtools_realloc(void* ptr, size_t n, unsigned int line, char* file); /* Version of realloc which keeps track of line and file where memory was reallocated */

void memtools_print_allocated(); /* print all currently allocated memory */
bool memtools_has_memory_been_violated(void* ptr); /* check if any over allocated segments are corrupted */
bool memtools_memory_comment(void* ptr, char* fmt, ...); /* add comment to memory */
bool memtools_is_valid_pointer(void* ptr); /* check if pointer is valid */
void memtools_memory_comment_copy(void* dest_block, void* src_block);

int memtools_wrapped_printf(char* fmt, ...);

#endif

