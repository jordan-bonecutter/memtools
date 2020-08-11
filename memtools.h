/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools.h  * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* 
 * Heavily inspired by Eskil Steenberg's talk (https://youtu.be/443UNeGrFoM?t=3164)
 * I have decided to make a similar tool to his for memory checking. Just for fun
 * (as always!)
 * */

#ifndef MEMTOOLS_H_INCLUDE_GUARD
  #define MEMTOOLS_H_INCLUDE_GUARD
  #ifdef MEMTOOLS
    #include "memtools_internal.h"

    #define malloc(n)     memtools_malloc (n, __LINE__, __FILE__)
    #define free(p)       memtools_free   (p, __LINE__, __FILE__)
    #define realloc(p, n) memtools_realloc(p, n, __LINE__, __FILE__)

    #define memprint()           memtools_print_allocated()
    #define memcomment(p, c)     memtools_memory_comment(p, c)
    #define memtest(p, comment)  if(memtools_is_valid_pointer(p)){\
                                    printf("memtools: memory tested in file %s at line %d was valid.\n",\
                                          __FILE__, __LINE__);\
                                 } else {\
                                    printf("memtools: memory tested in file %s at line %d was invalid.\n", \
                                           __FILE__, __LINE__);\
                                 } if(comment){\
                                      printf("\t(%s)\n", comment);\
                                 }
    #define memviolated(p, comment) if(!memtools_has_memory_been_violated(p)){\
                                    printf("memtools: memory tested in file %s at line %d has not been violated.\n",\
                                          __FILE__, __LINE__);\
                                 } else {\
                                    printf("memtools: memory tested in file %s at line %d has been violated.\n", \
                                           __FILE__, __LINE__);\
                                 } if(comment){\
                                      printf("\t(%s)\n", comment);\
                                 }
  #else
    #define memprint()
    #define memcomment(p, c)
    #define memtest(p, comment)
  #endif
#endif

