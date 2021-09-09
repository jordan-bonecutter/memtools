/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools_memory_interface.c * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define MAGIC_NUMBER 0xEC5EE674CA4A4A96

typedef struct{
  unsigned int line;
  uint8_t* memstart;
  size_t n;
  char **comments, *file, *alloc_type;
  unsigned int n_comments;
}memtools_allocation;

typedef struct{
  unsigned n_allocations;
  memtools_allocation* allocations;
}memtools_memory_interface;

typedef struct{
  bool is_valid_ptr, shifted_ptr;
  size_t n_bytes;
  void* memstart;
}memtools_free_info;

memtools_memory_interface* memtools_memory_interface_create(){
  return (memtools_memory_interface*)NULL;
}

/* malloc w/ 64 bit header and footer */
static inline void over_malloc(size_t n, memtools_allocation* curr){
  /* ensure footer is 64-bit aligned */
  unsigned long aligned_n = n + (n&7);
  curr->memstart = malloc(aligned_n + sizeof(uint64_t)*2) + sizeof(uint64_t);
  curr->n = n;

  /* write magic number to header and footer */
  *(((uint64_t*)curr->memstart) - 1) = MAGIC_NUMBER;
  *((uint64_t*)(curr->memstart + aligned_n)) = MAGIC_NUMBER;
}

memtools_allocation* memtools_memory_interface_add_allocation(memtools_memory_interface** interface, size_t n){
  memtools_memory_interface *interface_cache;
  memtools_allocation *ret;
  if(!*interface){
    *interface = malloc(sizeof **interface);
    interface_cache = *interface;
    interface_cache->n_allocations = 1;
    interface_cache->allocations = malloc(sizeof *interface_cache->allocations);
  } else {
    interface_cache = *interface;
    interface_cache->allocations = realloc(interface_cache->allocations,
                                          (sizeof *interface_cache->allocations)*(interface_cache->n_allocations + 1));
    ++interface_cache->n_allocations;
  }
  ret = interface_cache->allocations + (interface_cache->n_allocations - 1);
  over_malloc(n, ret);
  return ret;
}

static bool pointer_contained_in_allocation(memtools_allocation* allocation, void* ptr){
  return ((uint8_t*)ptr >= allocation->memstart) && ((uint8_t*)ptr <= allocation->memstart + allocation->n - 1);
}

memtools_allocation* memtools_memory_interface_get_allocation_for_pointer(memtools_memory_interface* interface, void* p){
  if(!interface){
    return NULL;
  }

  memtools_allocation* iterator;
  for(iterator = interface->allocations; iterator != interface->allocations + interface->n_allocations; ++iterator){
    if(pointer_contained_in_allocation(iterator, p)){
      return iterator;
    }
  }

  return NULL;
}

memtools_free_info
memtools_memory_interface_destroy_allocation_by_pointer(memtools_memory_interface** interface, void* ptr){
  int i;
  memtools_memory_interface *interface_cache = *interface;
  memtools_allocation *iterator;
  memtools_free_info ret;
  char** comment;

  ret.is_valid_ptr = false;
  ret.shifted_ptr = false;
  ret.n_bytes = 0;
  ret.memstart = 0;
  if(!interface){
    return ret;
  }
  for(i = 0, iterator = interface_cache->allocations;
      iterator != interface_cache->allocations + interface_cache->n_allocations;
      ++iterator, ++i)
  {
    if(pointer_contained_in_allocation(iterator, ptr)){
      ret.is_valid_ptr = true;
      break;
    }
  }

  if(!ret.is_valid_ptr){
    return ret;
  }

  if(ptr != iterator->memstart){
    ret.shifted_ptr = true;
  }
  ret.memstart = iterator->memstart;
  ret.n_bytes = iterator->n;
  free(iterator->memstart - sizeof(uint64_t));

  for(comment = iterator->comments; comment != iterator->comments + iterator->n_comments; ++comment){
    free(*comment);
  }
  if(iterator->comments){
    free(iterator->comments);
  }

  /* erase free'd block by shifting down all the next allocations */
  for(; i < interface_cache->n_allocations-1; ++i){
    interface_cache->allocations[i] = interface_cache->allocations[i+1];
  }
  --interface_cache->n_allocations;
  if(interface_cache->n_allocations == 0){
    free(*interface);
    *interface = NULL;
  } else {
    interface_cache->allocations = realloc(interface_cache->allocations,
                        (sizeof **interface) + (sizeof *(*interface)->allocations)*(*interface)->n_allocations);
  }

  return ret;
}

void memtools_memory_interface_for_each(memtools_memory_interface *interface, void (*for_each)(memtools_allocation*)){
  memtools_allocation *iterator;

  if(!interface){
    return;
  }

  for(iterator = interface->allocations; iterator != interface->allocations + interface->n_allocations; ++iterator){
    for_each(iterator);
  }
}

