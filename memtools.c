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
#define MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE 1000
#define MEMTOOLS_WPRINTF_BUFFER_SIZE        1000

char ALLOC_TYPE_MALLOC[]  = "malloc ";
char ALLOC_TYPE_REALLOC[] = "realloc";

typedef struct{
  unsigned int line;
  uint8_t* memstart;
  size_t n;
  char **comments, *file, *alloc_type;
  unsigned int n_comments;
}memtools_memory_allocation;

/* allocated memory data structure */
/* TODO: convert this to a binary tree for fatser lookup */
size_t total_allocated_bytes = 0;
unsigned int n_allocations = 0;
memtools_memory_allocation* memory_allocations = NULL;

/* mutex lock for multithreaded applications */
pthread_mutex_t memory_allocations_lock = PTHREAD_MUTEX_INITIALIZER;

/* print memtools before formatted string */
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
  /* ensure footer is 64-bit aligned */
  unsigned long aligned_n = n + (n&7);
  curr->memstart = malloc(aligned_n + sizeof(uint64_t)*2) + sizeof(uint64_t);
  curr->n = n;

  /* write magic number to header and footer */
  *(((uint64_t*)curr->memstart) - 1) = MAGIC_NUMBER;
  *((uint64_t*)(curr->memstart + aligned_n)) = MAGIC_NUMBER;
}

/* memtools version of malloc */
void* memtools_malloc(size_t n, unsigned int line, char* file){
  pthread_mutex_lock(&memory_allocations_lock);

  /* add more memory for new malloc */
  ++n_allocations;
  memory_allocations = realloc(memory_allocations, (sizeof *memory_allocations)*n_allocations);
  memtools_memory_allocation* curr = memory_allocations + n_allocations - 1;

  /* initialize current allocation */
  curr->line = line;
  curr->file = file;
  curr->alloc_type = ALLOC_TYPE_MALLOC;
  curr->comments = NULL;
  curr->n_comments = 0;
  over_malloc(n, curr);
  total_allocated_bytes += n;
  pthread_mutex_unlock(&memory_allocations_lock);

  return curr->memstart;
}

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

  /* nitialize 2 va_list in case we need to call vsnprintf again if
   * the buffer is too small */
  va_start(args1, fmt);
  va_copy(args2, args1);

  /* use vsnprintf to get formatted string from user */
  buffer = malloc((sizeof *buffer)*MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE);
  n = vsnprintf(buffer, MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE*(sizeof *buffer), fmt, args1);
  va_end(args1);

  /* always makes sense to realloc so we're not over allocating */
  buffer = realloc(buffer, (sizeof *buffer)*(n+1));

  /* if the buffer was initally too small, try vsnprintf again */
  if(n+1 > MEMTOOLS_MEMORY_COMMENT_BUFFER_SIZE){
    vsnprintf(buffer, n+1, fmt, args2);
  }
  va_end(args2);
  curr->n_comments++;
  curr->comments = realloc(curr->comments, (sizeof *curr->comments)*curr->n_comments);
  curr->comments[curr->n_comments - 1] = buffer;
  pthread_mutex_unlock(&memory_allocations_lock);
}

/* check if ptr is in any of the current allocations */
bool memtools_is_valid_pointer(void* ptr){
  memtools_memory_allocation* curr; 

  pthread_mutex_lock(&memory_allocations_lock);
  curr = get_allocation_for_pointer(ptr);
  pthread_mutex_unlock(&memory_allocations_lock);

  /* curr will either be NULL or non-null. If it's NULL 
   * then (bool) curr = false, otherwise it's true which
   * is exactly what we want. */
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
  char** comment;
  print_wrapped("%s:%zu bytes allocated at %p allocated in file %s at line %d\n", 
        allocation->alloc_type, allocation->n, allocation->memstart + allocation->n,
        allocation->file, allocation->line);

  if(allocation_has_been_violated(allocation)){
    printf("\t %s! MEMORY HAS BEEN VIOLATED !%s\n", "\033[31m", "\033[0m");
  }
  for(comment = allocation->comments; comment != allocation->comments + allocation->n_comments; ++comment){
    printf("\t(%s)\n", *comment);
  }
}

/* print all allocations */
void memtools_print_allocated(){
  pthread_mutex_lock(&memory_allocations_lock);
  print_wrapped("allocated %zu bytes in %d blocks\n", total_allocated_bytes, n_allocations);
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
  char **comment;

  pthread_mutex_lock(&memory_allocations_lock);

  assert(ptr);
  /* we can't use get_allocation_for pointer because we need 
   * the block index to erase the block from the array. */
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
   
  for(comment = curr->comments; comment != curr->comments + curr->n_comments; ++comment){
    free(*comment);
  }
  if(curr->comments){
    free(curr->comments);
  }

  /* erase free'd block by shifting down all the next allocations */
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

  /* since you can use realloc as malloc if ptr is
   * NULL, we'll just use malloc for that. The only 
   * minor issue w/ this is that memtools will report
   * the allocation as a malloc rather than a realloc
   * but this actually makes some sense as it's not 
   * actually reallocating memory so I think I won't
   * fix this 'bug' */
  if(!ptr){
    return memtools_malloc(n, line, file);
  }

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

/* append a tab to nonempty printf statements */
int memtools_wrapped_printf(char* fmt, ...){
  char* buffer;
  va_list args1, args2;
  int n;

  va_start(args1, fmt);
  va_copy(args2, args1);

  /* see how long buffer is */
  buffer = malloc((sizeof *buffer)*MEMTOOLS_WPRINTF_BUFFER_SIZE);
  n = vsnprintf(buffer, MEMTOOLS_WPRINTF_BUFFER_SIZE, fmt, args1);
  va_end(args1);

  /* if there's nothing to print, don't bother printing 
   * a tab or a newline as that will muck with the formatting */
  if(n == 0){
    va_end(args2);
    free(buffer);
    return 0;
  } 

  /* otherwise, make sure we captured the whole thing in buffer
   * so that we can print it all */
  if(n+1 > MEMTOOLS_WPRINTF_BUFFER_SIZE){
    buffer = realloc(buffer, (sizeof *buffer)*(n+1));
    vsnprintf(buffer, n+1, fmt, args2);
  }
  va_end(args2);
  n = printf("\t%s\n", buffer);
  free(buffer);
  return n;
}

static void comment_copy(char **dest, char *src){
  unsigned int n;

  n = strlen(src);
  *dest = malloc((sizeof **dest)*(n+1));
  strncpy(*dest, src, n);
}

void memtools_memory_comment_copy(void* dest_block, void* src_block){
  memtools_memory_allocation *dest_allocation, *src_allocation;
  int n_original_comments;
  char **dest_comment, **src_comment;

  src_allocation = get_allocation_for_pointer(src_block);
  if(!src_allocation){
    print_wrapped("Tried to copy comments from pointer at %p but pointer was invalid.\n", src_block);
    exit(0);
  }

  dest_allocation = get_allocation_for_pointer(dest_block);
  if(!dest_allocation){
    print_wrapped("Tried to copy comments from pointer at %p to pointer at %p but destination pointer was invalid.\n", 
                  src_block, dest_block);
    exit(0);
  }

  n_original_comments = dest_allocation->n_comments;
  dest_allocation->n_comments += src_allocation->n_comments;
  dest_allocation->comments = realloc(dest_allocation->comments, 
                                      (sizeof *dest_allocation->comments)*dest_allocation->n_comments);
  for(src_comment = src_allocation->comments, dest_comment = dest_allocation->comments + n_original_comments;
      src_comment != src_allocation->comments + src_allocation->n_comments;
      ++src_comment, ++dest_comment){
    comment_copy(dest_comment, *src_comment);
  }
}

