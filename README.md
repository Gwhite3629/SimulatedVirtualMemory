This is a library for a simple dynamic memory allocator. It is designed with
a 'kernel heap' as highest level of organization. From this regions are created
with sizes of 4kB. This is to simulate retrieving a page from the lower level
virtual memory system. When a user requests memory, the region is split and the
library carves out a section for the user to have. Each requested pointer has
an associated smart pointer. This allows containers to be made with the library
that ensure safe memory operations at runtime.

In order to implement this as a full kernel memory management system there would 
need to be a master kernel module running a program containing the kernel heap. 
Each userspace program would call a function that engages an interrupt and 
requests memory from the kernel module.

Library contains 3 functions for users:
1. new (void *ptr, int n, type);
2. alt (void *ptr, int n, type);
3. del (void *ptr);

new allocates a block of memory with 'n' elemnts of 'type'
The ptr argument is what the user uses. The macro is not intended to 
assign to the pointer.

alt changes the size of the allocated memory. Data is preservedwithin 
the range of new or old sizes. Works like realloc

del frees the ptr. It doesn't work exactly like free however.
The function clears the memory region in the heap and combines it with 
surrounding free regions.

Pointer values should not be manually updated. Arithmetic can be performed 
but intermediate results should be stored seperately.
The library can only find allocated memory if the original pointer value is
maintained.

make creates a build directory containing a shared object file for the library
and two non-instrumented test programs.
The test script compiles the two programs with instrumentation and logging 
enabled.

TODO:
1. Integrate into operating system with underlying virtual memory system
2. Turn into a full slab allocator with smart pointer checks and process specific address space
3. Expand profiling options and make them modular
