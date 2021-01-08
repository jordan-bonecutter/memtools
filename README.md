# memtools - Responsibly allocating memory in C
## Introduction

Let's face it, everybody hates using `malloc()`, `realloc()`, and `free()`. The goal of memtools is to alleviate the headache that inevitably comes with allocating 
your own memory. memtools tackles this by giving the user a few new powerful tools: `memprint()`, `memcomment()`, `memtest()`, and `memviolated()`. Behind the
scenes memtools replaces `malloc()`, `realloc()`, and `free()` with its own wrappers of these functions. These wrappers keep a table of all your memory allocations
making these tools possible. When you compile with `-DMEMTOOLS`, memtools will replace the standard allocation functions with the wrappers and enable its tools.
When you switch off the `MEMTOOLS` macro, all of the tools disappear and the wrappers are restored to the original functions.

## How to use memtools

The memtools philospphy is that all of its tools should *NOT* affect the logic of your program. This means all the tools can be though of as `void` returning 
functions. This may seem like a strange decision, as at first glance the following statement seems quite powerful:
```c
/* DO NOT DO THIS, THIS IS BAD!!! */
if(pointer_is_valid(ptr)){
        dereference_my_pointer(ptr);
} else {
        try_to_fix_bad_pointer(&ptr);
}
```        
While memtools could indeed provide the above functionality (and does if you want to dig deep into the weeds), this would be catastrophic for the development 
process, only creating further headaches. memtools is not built like this because it is better to fix code at compile time rather than at run time. This means 
that writing code trying to fix other code is a terrible idea, and you really shouldn't do it. The other reason is that you want to be able to turn off memtools 
when it's time to compile the production version of your code (memtools introduces non-trivial overhead that is acceptable for debugging but would likely 
make your customers sad). With the above code, it's not clear what part of it should be turned off for production, so that makes it even more messy!

Ok, so you know how not to use memtools, so how should you use it? First, I'll explain what each tool does and then we'll look at some examples.

1. `memprint()` - prints all allocated memory blocks along with any comments and whether this memory has been 'violated' (we'll talk about violation later)
2. `memcomment(ptr, comment)` - attaches a comment to a block of memory that will be printed out in memprint.
3. `memtest(ptr, comment)` - takes in a pointer and prints whether or not this pointer points to allocated memory.
4. `memviolated(ptr, comment)` - takes in a pointer and prints whether the memory block this pointer is inside has been 'violated'. memtools does memory violation checking
by over-allocating memory blocks and storing a special number at the ends of the block. If memtools detects this memory has been overwritten, it decides that
this block has been 'violated'. While this isn't a catch-all solution (it's possible the same number is overwritten or that memory outside the header and footer
is violated) it certainly is helpful.

Now that we know about all of the tools, let's look at an example usage:
```c
#include <memtools.h>
#include <stdlib.h>
        
int main(int argc, const char** argv){
        int *array, N, *head;
            
        N = atoi(argv[1]);
            
        array = malloc((sizeof *array)*N);
        memcomment(array, "This is my array with %d integers", N);
            
        for(head = array; head != array + N; ++head){
            memtest(head, "Checking head pointer on %d iteration", (head - array)/(sizeof *array));
            *head = rand();
        }
            
        free(array);
        memprint();
}
```        
A few comments on this code. First, you can see that memcomment not only allows string literals, but also `printf` style formattable strings which is quite handy!
Second, I like to use memtest on every pointer dereference in my code. This allows an extremely fine granularity on where exactly I caused a segfault. Finally, I
like to end the program with a call to memprint() when I know that all of my memory should be deallocated. If it doesn't report 0 bytes in 0 blocks, then I know
that I have a memory leak somewhere in my code (and it will tell me where!).

## Plans for the future

Right now, memtools uses an aray to hold all the allocations which is not ideal for search. I plan to move to a binary tree where the integer value of the pointer
is used as its key. This should provide a much faster lookup time (for large amounts of allocations). I would also like to support multiple comments and comment
deletion for memory allocations (though I'm not quite sure how to move forward from a user standpoint here). If you'd like to help contribute, please contact me!
