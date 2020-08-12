/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools.c  * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define MAGIC_NUMBER 0xEC5EE674CA4A4A96

char ALLOC_TYPE_MALLOC[]  = "malloc ";
char ALLOC_TYPE_REALLOC[] = "realloc";

typedef struct{
  unsigned int line;
  uint8_t* memstart;
  size_t n;
  char* comment, *file, *alloc_type;
}memtools_memory_allocation;

size_t total_allocated_bytes = 0;
unsigned int n_allocations = 0;
memtools_memory_allocation* memory_allocations = NULL;
pthread_mutex_t memory_allocations_lock = PTHREAD_MUTEX_INITIALIZER;

void print_wrapped(const char* format, ...){
  printf("memtools: ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

/* check if allocation has been violated by looking at header and footer */
static bool allocation_has_been_violated(memtools_memory_allocation* allocation){
  return *((uint64_t*)(allocation->memstart + allocation->n + (allocation->n&0x7))) != MAGIC_NUMBER || *(((uint64_t*)allocation->memstart)-1) != MAGIC_NUMBER;
}

/* check if pointer contained in allocation */
static bool pointer_contained_in_allocation(memtools_memory_allocation* allocation, void* ptr){
  return ((uint8_t*)ptr >= allocation->memstart) && ((uint8_t*)ptr <= allocation->memstart + allocation->n);
}

/* return either allocation containing ptr or NULL if ptr is invalid */
static memtools_memory_allocation* get_allocation_for_pointer(void* ptr){
  memtools_memory_allocation *curr;

  for(curr = memory_allocations; curr != memory_allocations + n_allocations; ++curr){
    if(pointer_contained_in_allocation(curr, ptr)){
      return curr;
    }
  }

  return NULL;
}

/* malloc w/ 64 bit header and footer */
static inline void over_malloc(size_t n, memtools_memory_allocation* curr){
  unsigned long aligned_n = n + (n&7);
  curr->memstart = malloc(aligned_n + sizeof(uint64_t)*2) + sizeof(uint64_t);
  curr->n = n;

  *(((uint64_t*)curr->memstart) - 1) = MAGIC_NUMBER;
  *((uint64_t*)(curr->memstart + aligned_n)) = MAGIC_NUMBER;
}

/* memtools version of malloc */
void* memtools_malloc(size_t n, unsigned int line, char* file){
  pthread_mutex_lock(&memory_allocations_lock);
  ++n_allocations;
  memory_allocations = realloc(memory_allocations, (sizeof *memory_allocations)*n_allocations);

  memtools_memory_allocation* curr = memory_allocations + n_allocations - 1;

  curr->line = line;
  curr->file = file;
  curr->alloc_type = ALLOC_TYPE_MALLOC;
  curr->comment = NULL;
  over_malloc(n, curr);
  total_allocated_bytes += n;
  pthread_mutex_unlock(&memory_allocations_lock);

  return curr->memstart;
}

#define MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE 1000

/* add a comment to current memory allocation */
void memtools_memory_comment(void* ptr, char* fmt, ...){
  memtools_memory_allocation* curr; 
  int n;
  char *buffer;
  va_list args1, args2;

  pthread_mutex_lock(&memory_allocations_lock);
  curr = get_allocation_for_pointer(ptr);
  if(!curr){
    print_wrapped("Tried to comment on pointer at %p but pointer was invalid.\n", ptr);
    exit(0);
  }

  /* use vsnprintf to get formatted string from user */
  buffer = malloc((sizeof *buffer)*MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE);
  va_start(args1, fmt);
  va_copy(args2, args1);
  n = vsnprintf(buffer, MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE*(sizeof *buffer), fmt, args1);
  va_end(args1);
  if(n > MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE){
    buffer = realloc(buffer, (sizeof *buffer)*(n+1));
    vsnprintf(buffer, n+1, fmt, args2);
  }
  va_end(args2);
  curr->comment = buffer;
  pthread_mutex_unlock(&memory_allocations_lock);
}

/* check if ptr is in any of the current allocations */
bool memtools_is_valid_pointer(void* ptr){
  memtools_memory_allocation* curr; 

  pthread_mutex_lock(&memory_allocations_lock);
  curr = get_allocation_for_pointer(ptr);
  pthread_mutex_unlock(&memory_allocations_lock);

  return (bool)curr;
}

/* check if the allocation containing ptr has been violated */
bool memtools_has_memory_been_violated(void* ptr){
  memtools_memory_allocation* curr; 
  bool has_memory_been_violated;

  pthread_mutex_lock(&memory_allocations_lock);
  curr = get_allocation_for_pointer(ptr);
  if(!curr){
    print_wrapped("Tried to violation check pointer at %p but pointer was invalid.\n", ptr);
    exit(0);
  }

  has_memory_been_violated = allocation_has_been_violated(curr);
  pthread_mutex_unlock(&memory_allocations_lock);

  return has_memory_been_violated;
}

/* print memtools_memory_allocation struct */
static void print_allocation(memtools_memory_allocation* allocation){
  print_wrapped("%s:%zu bytes allocated at %p allocated in file %s at line %d\n", 
        allocation->alloc_type, allocation->n, allocation->memstart + allocation->n,
        allocation->file, allocation->line);

  if(allocation_has_been_violated(allocation)){
    printf("\t %s! MEMORY HAS BEEN VIOLATED !%s\n", "\033[31m", "\033[0m");
  }
  if(allocation->comment){
    printf("\t(%s)\n", allocation->comment);
  }
}

/* print all allocations */
void memtools_print_allocated(){
  pthread_mutex_lock(&memory_allocations_lock);
  print_wrapped("allocated %llu bytes in %d blocks\n", total_allocated_bytes, n_allocations);
  memtools_memory_allocation* curr = memory_allocations;
  if(!curr){
    return;
  }
  for(; curr != memory_allocations + n_allocations; print_allocation(curr++));

  pthread_mutex_unlock(&memory_allocations_lock);
}

/* memtools version of free */
void memtools_free(void* ptr, unsigned line, char* file){
  memtools_memory_allocation* curr; 
  int i;
  bool is_valid_ptr = false;

  pthread_mutex_lock(&memory_allocations_lock);

  assert(ptr);
  for(i = 0, curr = memory_allocations; curr != memory_allocations + n_allocations; ++curr, ++i){
    if(pointer_contained_in_allocation(curr, ptr)){
      is_valid_ptr = true;
      break;
    }
  }

  if(!is_valid_ptr){
    print_wrapped("Tried to free pointer at %p in file %s at line %d but pointer was invalid\n", ptr, file, line);
    exit(0);
  }

  if(ptr != curr->memstart){
    print_wrapped("warning - you are freeing allocation at %p in file %s at line %d using shifted pointer %p\n", 
                  curr->memstart - sizeof(uint64_t), file, line, ptr);
  }
  total_allocated_bytes -= curr->n;
  free(curr->memstart - sizeof(uint64_t));
  if(curr->comment){
    free(curr->comment);
  }
  for(; i < n_allocations-1; ++i){
    memory_allocations[i] = memory_allocations[i+1];
  }
  --n_allocations;
  memory_allocations = realloc(memory_allocations, (sizeof *memory_allocations)*n_allocations);
  pthread_mutex_unlock(&memory_allocations_lock);
}

/* realloc with 64 bit header and footer */
static inline void over_realloc(size_t n, memtools_memory_allocation* curr){
  unsigned long aligned_n = n + (n&7);
  curr->memstart = realloc(curr->memstart - sizeof(uint64_t), aligned_n + sizeof(uint64_t)*2) + sizeof(uint64_t);
  curr->n = n;

  *(((uint64_t*)curr->memstart) - 1) = MAGIC_NUMBER;
  *((uint64_t*)(curr->memstart + aligned_n)) = MAGIC_NUMBER;
}

/* memtools version of realloc */
void* memtools_realloc(void* ptr, size_t n, unsigned int line, char* file){
  memtools_memory_allocation* curr; 

  pthread_mutex_lock(&memory_allocations_lock);
  curr = get_allocation_for_pointer(ptr);
  if(!curr){
    print_wrapped("Tried to realloc pointer at %p in file %s at line %d but pointer was invalid.\n", ptr, file, line);
    exit(0);
  }

  total_allocated_bytes = total_allocated_bytes - curr->n + n;
  over_realloc(n, curr);
  curr->line = line;
  curr->file = file;
  curr->alloc_type = ALLOC_TYPE_REALLOC;
  pthread_mutex_unlock(&memory_allocations_lock);
  return curr->memstart;
}

