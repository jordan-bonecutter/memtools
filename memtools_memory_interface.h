/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools_memory_interface.h * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef memtools_memory_interface_INCLUDE_GUARD
#define memtools_memory_interface_INCLUDE_GUARD

typedef struct{
  unsigned int line;
  uint8_t* memstart;
  size_t n;
  char **comments, *file, *alloc_type;
  unsigned int n_comments;
}memtools_allocation;

typedef struct{
  bool is_valid_ptr, shifted_ptr;
  size_t n_bytes;
  void* memstart;
}memtools_free_info;

typedef void* memtools_memory_interface;

memtools_memory_interface* memtools_memory_interface_create();
memtools_allocation* memtools_memory_interface_add_allocation(memtools_memory_interface**, size_t n);
memtools_allocation* memtools_memory_interface_get_allocation_for_pointer(memtools_memory_interface*, void*);
memtools_free_info memtools_memory_interface_destroy_allocation_by_pointer(memtools_memory_interface**, void*);
void memtools_memory_interface_for_each(memtools_memory_interface*, void (*for_each)(memtools_allocation*));

#endif

