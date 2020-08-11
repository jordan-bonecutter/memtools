# memtools
## Responsibly allocating memory in C

Let's face it, everybody hates using `malloc()` and `free()`. That's why I started working on memtools, to make it easier!
To use memtools, you need only to include the header file `memtools.h` and link with `memtools.o`. I'll show you how memtools can help you!

The first awesome thing that memtools does is it keeps track of all your active memory allocations. An active memory allocation is one from a malloc
or a realloc which has not yet been freed. And even better, it does it all behind the scenes by transparently replacing malloc when the `MEMTOOLS` macro
is defined. When it's switched off, malloc and free will work as normal! So allocating memory with memtools is as simple as:

    uint8_t* buffer = malloc((sizeof *buffer)*BUFFER_LENGTH);
    
To print the memory you have currently allocated, just call the `memprint()` macro. I advise only using the maros as these disappear when `MEMTOOLS` is not defined, 
meaning that you don't need to change your code when you want to compile it for production. You can also comment on memory using:

    memcomment(ptr, "Pointer to my data structure");
    
This will attach a comment to the memory block which can be seen when calling `memprint()`. Memtools also over allocates memory, leaving a 64 bit header and footer 
for every memory allocation. It stores a special bit pattern which is used to detect erroneous memory writes. When calling `memprint()`, it will alert you if
there were memory writes in the special header and footer so that you can find and fix your bugs.
