#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils.h"
#include "memory.h"

const char *stat_names[12] = {
    "UNALLOCATED",
    "RESERVED",
    "CULLED",
    "AVAILABLE"
};

int ret = SUCCESS;

heap_t *global_heap = NULL;
FILE *log_file = NULL;

__attribute__ ((constructor)) static inline void init(void)
{
	global_heap = create(ALIGN, 1*ALIGN);
	VALID(global_heap, MEM_CODE, ALLOCATION_ERROR);
	return;
exit:
	abort();
}

__attribute__ ((destructor)) static inline void end(void)
{
	destroy(global_heap);
}

// Create initial heap with kernel region and user region 1
inline heap_t *create(int alignment, int size)
{
    heap_t h;

    // Kernel region info
    region_t kheap;
    void *base_kheap = NULL;
    smart_ptr heap_reg_info;
    smart_ptr heap_ptr;
    smart_ptr reg_arr;
    smart_ptr heap_free_space;
    smart_ptr heap_chunk_arr;

    // User region 1 info
    region_t reg1;
    void *base_reg1 = NULL;
    smart_ptr reg_info;
    smart_ptr free_space;
    smart_ptr chunk_arr;

    #if WINDOWS
        base_kheap = (void *)_aligned_malloc(size, alignment);
        VALID(base_kheap, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_kheap, 0, size);

        base_reg1 = (void *)_aligned_malloc(size, alignment);
        VALID(base_reg1, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_reg1, 0, size);
    #else
        base_kheap = (void *)aligned_alloc(alignment, size);
        VALID(base_kheap, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_kheap, 0, size);

        base_reg1 = (void *)aligned_alloc(alignment, size);
        VALID(base_reg1, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_reg1, 0, size);
    #endif


    // Initialize kernel region

    // Smart ptr for kernel region info
    heap_reg_info.size = REGION_INFO_SIZE;
    heap_reg_info.flag = RESERVED;
    heap_reg_info.base = base_kheap;
    heap_reg_info.addr = (char *)base_kheap + CHUNK_INFO_SIZE;

    // Smart ptr for heap info
    heap_ptr.size = HEAP_INFO_SIZE;
    heap_ptr.flag = RESERVED;
    heap_ptr.base = base_kheap;
    heap_ptr.addr = (char *)heap_reg_info.addr +  REGION_INFO_SIZE + CHUNK_INFO_SIZE;

    // Smart ptr for region array
    reg_arr.size = 2 * REGION_ARR;
    reg_arr.flag = RESERVED;
    reg_arr.base = base_kheap;
    reg_arr.addr = (char *)heap_ptr.addr + HEAP_INFO_SIZE + CHUNK_INFO_SIZE;
    
    // Smart ptr for chunk array
    heap_chunk_arr.size = 5 * CHUNK_ARR;
    heap_chunk_arr.flag = RESERVED;
    heap_chunk_arr.base = base_kheap;
    heap_chunk_arr.addr = ((char *)base_kheap + size) - heap_chunk_arr.size;

    // Region info
    kheap.alloc_size =  size;
    kheap.used_size =  REGION_INFO_SIZE
                    +  HEAP_INFO_SIZE
                    +  5 * CHUNK_INFO_SIZE
                    +  2 * REGION_ARR
                    +  5 * CHUNK_ARR; 
    kheap.n_chunks = 5;
    kheap.base_addr = base_kheap;
    kheap.chunks = heap_chunk_arr.addr;

    // Heap info
    h.alignment = alignment;
    h.n_regions = 2;
    h.regions = reg_arr.addr;

    // Smart ptr for remaining free space
    heap_free_space.size = (size - kheap.used_size);
    heap_free_space.flag = UNALLOCATED;
    heap_free_space.base = base_kheap;
    heap_free_space.addr = (char *)reg_arr.addr + 2 * REGION_ARR + CHUNK_INFO_SIZE;

    // Assign chunks array
    kheap.chunks[0] = (smart_ptr *)((char *)heap_chunk_arr.addr - CHUNK_INFO_SIZE); // Chunk array always first chunk
    kheap.chunks[1] = (smart_ptr *)((char *)heap_reg_info.addr - CHUNK_INFO_SIZE);
    kheap.chunks[2] = (smart_ptr *)((char *)heap_ptr.addr - CHUNK_INFO_SIZE);
    kheap.chunks[3] = (smart_ptr *)((char *)reg_arr.addr - CHUNK_INFO_SIZE);
    kheap.chunks[4] = (smart_ptr *)((char *)heap_free_space.addr - CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[0], &heap_chunk_arr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[1], &heap_reg_info, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[2], &heap_ptr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[3], &reg_arr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[4], &heap_free_space, CHUNK_INFO_SIZE);

    // Assign region info
    memcpy(kheap.chunks[1]->addr, &kheap, REGION_INFO_SIZE);
    // Clear freespace
    memset(kheap.chunks[4]->addr, 0, kheap.chunks[4]->size);

    // Initialize user region 1

    // Smart ptr for user region 1 info
    reg_info.size = REGION_INFO_SIZE;
    reg_info.flag = RESERVED;
    reg_info.base = base_reg1;
    reg_info.addr = (char *)base_reg1 + CHUNK_INFO_SIZE;

    // Smart ptr for chunk array
    chunk_arr.size = 3 * CHUNK_ARR;
    chunk_arr.flag = RESERVED;
    chunk_arr.base = base_reg1;
    chunk_arr.addr = ((char *)base_reg1 + size) - chunk_arr.size;

    // Region info
    reg1.alloc_size =  size;
    reg1.used_size =  REGION_INFO_SIZE
                    +  3 * CHUNK_INFO_SIZE
                    +  3 * CHUNK_ARR; 
    reg1.n_chunks = 3;
    reg1.base_addr = base_reg1;
    reg1.chunks = chunk_arr.addr;

    // Smart ptr for remaining free space
    free_space.size = (size - reg1.used_size);
    free_space.flag = UNALLOCATED;
    free_space.base = base_reg1;
    free_space.addr = (char *)reg_info.addr + REGION_INFO_SIZE + CHUNK_INFO_SIZE;

    // Assign chunks array
    reg1.chunks[0] = (smart_ptr *)((char *)chunk_arr.addr - CHUNK_INFO_SIZE); // Chunk array always first chunk
    reg1.chunks[1] = (smart_ptr *)((char *)reg_info.addr - CHUNK_INFO_SIZE);
    reg1.chunks[2] = (smart_ptr *)((char *)free_space.addr - CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[0], &chunk_arr, CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[1], &reg_info, CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[2], &free_space, CHUNK_INFO_SIZE);

    // Assign region info
    memcpy(reg1.chunks[1]->addr, &reg1, REGION_INFO_SIZE);
    // Clear freespace
    memset(reg1.chunks[2]->addr, 0, reg1.chunks[2]->size);

    // Assign region array
    h.regions[0] = (region_t *)(heap_reg_info.addr);
    h.regions[1] = (region_t *)(reg_info.addr);

    // Assign heap info
    memcpy((heap_t *)kheap.chunks[2]->addr, &h, HEAP_INFO_SIZE);

    return (heap_t *)kheap.chunks[2]->addr;

exit:
    return NULL;
}

inline void destroy(heap_t *h)
{
    if (h) {
        for (int i = h->n_regions; i > 0; i--) {
            if (h->regions[i-1]->base_addr) {
                free(h->regions[i-1]->base_addr);
            }
        }
    }
}

inline heap_t *grow_kheap(heap_t *h)
{
    heap_t new_h;

    // Kernel region info
    region_t kheap;
    void *base_kheap = NULL;
    smart_ptr heap_reg_info;
    smart_ptr heap_ptr;
    smart_ptr reg_arr;
    smart_ptr heap_free_space;
    smart_ptr heap_chunk_arr;

    int alignment = h->alignment;
    int size = h->regions[0]->alloc_size + alignment;

    #if WINDOWS
        base_kheap = (void *)_aligned_malloc(size, alignment);
        VALID(base_kheap, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_kheap, 0, size);
    #else
        base_kheap = (void *)aligned_alloc(alignment, size);
        VALID(base_kheap, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_kheap, 0, size);
    #endif

    // Initialize kernel region

    // Smart ptr for kernel region info
    heap_reg_info.size = REGION_INFO_SIZE;
    heap_reg_info.flag = RESERVED;
    heap_reg_info.base = base_kheap;
    heap_reg_info.addr = (char *)base_kheap + CHUNK_INFO_SIZE;

    // Smart ptr for heap info
    heap_ptr.size = HEAP_INFO_SIZE;
    heap_ptr.flag = RESERVED;
    heap_ptr.base = base_kheap;
    heap_ptr.addr = (char *)heap_reg_info.addr +  REGION_INFO_SIZE + CHUNK_INFO_SIZE;

    // Smart ptr for region array
    reg_arr.size = h->n_regions * REGION_ARR;
    reg_arr.flag = RESERVED;
    reg_arr.base = base_kheap;
    reg_arr.addr = (char *)heap_ptr.addr + HEAP_INFO_SIZE + CHUNK_INFO_SIZE;
    
    // Smart ptr for chunk array
    heap_chunk_arr.size = 5 * CHUNK_ARR;
    heap_chunk_arr.flag = RESERVED;
    heap_chunk_arr.base = base_kheap;
    heap_chunk_arr.addr = ((char *)base_kheap + size) - heap_chunk_arr.size;

    // Region info
    kheap.alloc_size =  size;
    kheap.used_size =  REGION_INFO_SIZE
                    +  HEAP_INFO_SIZE
                    +  5 * CHUNK_INFO_SIZE
                    +  h->n_regions * REGION_ARR
                    +  5 * CHUNK_ARR; 
    kheap.n_chunks = 5;
    kheap.base_addr = base_kheap;
    kheap.chunks = heap_chunk_arr.addr;

    // Heap info
    new_h.alignment = alignment;
    new_h.n_regions = h->n_regions;
    new_h.regions = reg_arr.addr;

    // Smart ptr for remaining free space
    heap_free_space.size = (size - kheap.used_size);
    heap_free_space.flag = UNALLOCATED;
    heap_free_space.base = base_kheap;
    heap_free_space.addr = (char *)reg_arr.addr + h->n_regions * REGION_ARR + CHUNK_INFO_SIZE;

    // Assign chunks array
    kheap.chunks[0] = (smart_ptr *)((char *)heap_chunk_arr.addr - CHUNK_INFO_SIZE); // Chunk array always first chunk
    kheap.chunks[1] = (smart_ptr *)((char *)heap_reg_info.addr - CHUNK_INFO_SIZE);
    kheap.chunks[2] = (smart_ptr *)((char *)heap_ptr.addr - CHUNK_INFO_SIZE);
    kheap.chunks[3] = (smart_ptr *)((char *)reg_arr.addr - CHUNK_INFO_SIZE);
    kheap.chunks[4] = (smart_ptr *)((char *)heap_free_space.addr - CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[0], &heap_chunk_arr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[1], &heap_reg_info, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[2], &heap_ptr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[3], &reg_arr, CHUNK_INFO_SIZE);
    memcpy(kheap.chunks[4], &heap_free_space, CHUNK_INFO_SIZE);

    // Assign region info
    memcpy(kheap.chunks[1]->addr, &kheap, REGION_INFO_SIZE);
    // Clear freespace
    memset(kheap.chunks[4]->addr, 0, kheap.chunks[4]->size);
    new_h.regions[0] = (region_t *)(heap_reg_info.addr);
    memcpy(((char *)new_h.regions + REGION_ARR), ((char *)h->regions + REGION_ARR), (h->n_regions-1)*REGION_ARR);
    memcpy((heap_t *)kheap.chunks[2]->addr, &new_h, HEAP_INFO_SIZE);

    free(h->regions[0]->base_addr);

    h = (heap_t *)(kheap.chunks[2]->addr);

    return h;

exit:
    return NULL;
}


inline void *alloc(heap_t *h, int n)
{
    int i = 0;
    void *ptr = NULL;
    int alloced = 0;
    
    for (i = 1; i < h->n_regions; i++) {
        if ((h->regions[i]->chunks[h->regions[i]->n_chunks-1]->size > (int)(n + CHUNK_INFO_SIZE + CHUNK_ARR)) & !F_CHECK(h->regions[i]->chunks[h->regions[i]->n_chunks-1])) {
            // Check passed, region space has enough room for:
            // 1. Smart ptr
            // 2. User data ptr
            // 3. New chunk arr ptr
            // Steps:
            // 1. Create new smart ptr
            // 2. Move back old chunk array and chunk array smart ptr
            // 3. Define second to last chunk arr as user ptr
            // 4. Define last as free ptr again
            // 5. Reduce size of free region
            smart_ptr new_free;
            smart_ptr new_chunk;
            smart_ptr usr_smart;

            new_free.addr = ((char *)h->regions[i]->chunks[h->regions[i]->n_chunks-1]->addr + n + CHUNK_INFO_SIZE);
            new_free.size = h->regions[i]->chunks[h->regions[i]->n_chunks-1]->size - (n + CHUNK_INFO_SIZE + CHUNK_ARR);
            new_free.base = h->regions[i]->base_addr;
            new_free.flag = UNALLOCATED;

            new_chunk.addr = (char *)h->regions[i]->chunks[0]->addr - CHUNK_ARR;
            new_chunk.size = (h->regions[i]->n_chunks + 1) * CHUNK_ARR;
            new_chunk.base = h->regions[i]->base_addr;
            new_chunk.flag = RESERVED;

            usr_smart.addr = h->regions[i]->chunks[h->regions[i]->n_chunks-1]->addr;
            usr_smart.size = n;
            usr_smart.base = h->regions[i]->base_addr;
            usr_smart.flag = RESERVED;

            memcpy(((char *)new_free.addr - CHUNK_INFO_SIZE), &new_free, CHUNK_INFO_SIZE);
            memcpy(((char *)usr_smart.addr - CHUNK_INFO_SIZE), &usr_smart, CHUNK_INFO_SIZE);
            memmove(((char *)new_chunk.addr), h->regions[i]->chunks, h->regions[i]->n_chunks * CHUNK_ARR);
            memcpy(((char *)new_chunk.addr - CHUNK_INFO_SIZE), &new_chunk, CHUNK_INFO_SIZE);
            h->regions[i]->n_chunks++;
            h->regions[i]->chunks = (smart_ptr **)new_chunk.addr;
            h->regions[i]->chunks[0] = (smart_ptr *)((char *)new_chunk.addr - CHUNK_INFO_SIZE);
            h->regions[i]->chunks[h->regions[i]->n_chunks-2] = (smart_ptr *)((char *)usr_smart.addr - CHUNK_INFO_SIZE);
            h->regions[i]->chunks[h->regions[i]->n_chunks-1] = (smart_ptr *)((char *)new_free.addr - CHUNK_INFO_SIZE);

            ptr = usr_smart.addr;
            alloced = 1;
            h->regions[i]->used_size += (n + CHUNK_INFO_SIZE + CHUNK_ARR);
            break;
        }
    }

    if (alloced == 0) {
        create_region(h, (((n + NEW_REGION_SIZE) / h->alignment) + 1)*h->alignment);
        ptr = alloc(h, n);
        return ptr;
    }

    return ptr;
}

void create_region(heap_t *h, int size)
{
    // New User Region
    region_t reg1;
    void *base_reg1 = NULL;
    smart_ptr reg_info;
    smart_ptr free_space;
    smart_ptr chunk_arr;

    #if WINDOWS
        base_reg1 = (void *)_aligned_malloc(size, alignment);
        VALID(base_reg1, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_reg1, 0, size);
    #else
        base_reg1 = (void *)aligned_alloc(h->alignment, size);
        VALID(base_reg1, MEM_CODE, ALLOCATION_ERROR);
	    memset(base_reg1, 0, size);
    #endif

    if ((h->regions[0]->alloc_size - h->regions[0]->used_size) < (int)REGION_ARR) {
        h = grow_kheap(h);
    }
    VALID(h, MEM_CODE, ALLOCATION_ERROR);

    // Initialize user region

    // Smart ptr for user region info
    reg_info.size = REGION_INFO_SIZE;
    reg_info.flag = RESERVED;
    reg_info.base = base_reg1;
    reg_info.addr = (char *)base_reg1 + CHUNK_INFO_SIZE;

    // Smart ptr for chunk array
    chunk_arr.size = 3 * CHUNK_ARR;
    chunk_arr.flag = RESERVED;
    chunk_arr.base = base_reg1;
    chunk_arr.addr = ((char *)base_reg1 + size) - chunk_arr.size;

    // Region info
    reg1.alloc_size =  size;
    reg1.used_size =  REGION_INFO_SIZE
                    +  3 * CHUNK_INFO_SIZE
                    +  3 * CHUNK_ARR; 
    reg1.n_chunks = 3;
    reg1.base_addr = base_reg1;
    reg1.chunks = chunk_arr.addr;

    // Smart ptr for remaining free space
    free_space.size = (size - reg1.used_size);
    free_space.flag = UNALLOCATED;
    free_space.base = base_reg1;
    free_space.addr = (char *)reg_info.addr + REGION_INFO_SIZE + CHUNK_INFO_SIZE;

    // Assign chunks array
    reg1.chunks[0] = (smart_ptr *)((char *)chunk_arr.addr - CHUNK_INFO_SIZE); // Chunk array always first chunk
    reg1.chunks[1] = (smart_ptr *)((char *)reg_info.addr - CHUNK_INFO_SIZE);
    reg1.chunks[2] = (smart_ptr *)((char *)free_space.addr - CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[0], &chunk_arr, CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[1], &reg_info, CHUNK_INFO_SIZE);
    memcpy(reg1.chunks[2], &free_space, CHUNK_INFO_SIZE);

    // Assign region info
    memcpy(reg1.chunks[1]->addr, &reg1, REGION_INFO_SIZE);
    // Clear freespace
    memset(reg1.chunks[2]->addr, 0, reg1.chunks[2]->size);

    // Assign region array
    smart_ptr new_free;

    new_free.addr = ((char *)h->regions[0]->chunks[h->regions[0]->n_chunks-1]->addr + REGION_ARR);
    new_free.base = base_reg1;
    new_free.flag = UNALLOCATED;
    new_free.size = (h->regions[0]->chunks[h->regions[0]->n_chunks-1]->size - REGION_ARR);

    h->n_regions++;
    h->regions[h->n_regions-1] = (region_t *)(reg_info.addr);

    memcpy(((char *)new_free.addr - CHUNK_INFO_SIZE), &new_free, CHUNK_INFO_SIZE);
    h->regions[0]->chunks[h->regions[0]->n_chunks-1] = (smart_ptr *)((char *)new_free.addr - CHUNK_INFO_SIZE);

    h->regions[0]->used_size += REGION_ARR;
    h->regions[0]->chunks[3]->size += REGION_ARR;

exit:
    return;
}

inline void cull(heap_t *h, void *ptr)
{
    int found = 0;
    int i, j = 0;

    for (i = 1; i < h->n_regions; i++) {
        for (j = 0; j < h->regions[i]->n_chunks; j++) {
            if (h->regions[i]->chunks[j]->addr == ptr) {
                found = 1;
                break;
            }
        }
        if (found == 1) {
            h->regions[i]->chunks[j]->flag = CULLED;
            h->regions[i]->used_size -= (h->regions[i]->chunks[j]->size + CHUNK_INFO_SIZE + CHUNK_ARR);
            break;
        }
    }

    if (((h->regions[i]->chunks[j]->flag == CULLED) & !F_CHECK(h->regions[i]->chunks[j-1])) |
        ((h->regions[i]->chunks[j]->flag == CULLED) & (h->regions[i]->chunks[j-1]->flag == CULLED))) {
        smart_ptr new_free;
        smart_ptr new_chunk;

        new_free.addr = h->regions[i]->chunks[j-1]->addr;
        new_free.base = h->regions[i]->base_addr;
        new_free.flag = UNALLOCATED;
        new_free.size = h->regions[i]->chunks[j-1]->size + CHUNK_INFO_SIZE + h->regions[i]->chunks[j]->size;
    
        new_chunk.addr = ((char *)h->regions[i]->chunks + CHUNK_ARR);
        new_chunk.base = h->regions[i]->base_addr;
        new_chunk.flag = RESERVED;
        new_chunk.size = h->regions[i]->chunks[0]->size - CHUNK_ARR;

        memmove(new_chunk.addr, h->regions[i]->chunks, (j) * CHUNK_ARR);
        h->regions[i]->chunks = (smart_ptr **)new_chunk.addr;
        h->regions[i]->chunks[0] = (smart_ptr *)((char *)new_chunk.addr - CHUNK_INFO_SIZE);
        h->regions[i]->chunks[j-1] = (smart_ptr *)((char *)new_free.addr - CHUNK_INFO_SIZE);
        memcpy(((char *)new_free.addr - CHUNK_INFO_SIZE), &new_free, CHUNK_INFO_SIZE);
        memcpy(((char *)new_chunk.addr - CHUNK_INFO_SIZE), &new_chunk, CHUNK_INFO_SIZE);

        h->regions[i]->n_chunks = h->regions[i]->n_chunks - 1;
        h->regions[i]->chunks[h->regions[i]->n_chunks-1]->size += CHUNK_ARR;
        j -= 1;
    }

    if ((!F_CHECK(h->regions[i]->chunks[j]) & !F_CHECK(h->regions[i]->chunks[j+1])) |
        (!F_CHECK(h->regions[i]->chunks[j]) & (h->regions[i]->chunks[j+1]->flag == CULLED)) |
        ((h->regions[i]->chunks[j]->flag == CULLED) & !F_CHECK(h->regions[i]->chunks[j+1])) |
        ((h->regions[i]->chunks[j]->flag == CULLED) & (h->regions[i]->chunks[j+1]->flag == CULLED))) {
        smart_ptr new_free;
        smart_ptr new_chunk;

        new_free.addr = ptr;
        new_free.base = h->regions[i]->base_addr;
        new_free.flag = UNALLOCATED;
        new_free.size = h->regions[i]->chunks[j+1]->size + CHUNK_INFO_SIZE + h->regions[i]->chunks[j]->size;
    
        new_chunk.addr = ((char *)h->regions[i]->chunks + CHUNK_ARR);
        new_chunk.base = h->regions[i]->base_addr;
        new_chunk.flag = RESERVED;
        new_chunk.size = h->regions[i]->chunks[0]->size - CHUNK_ARR;

        memcpy(((char *)new_free.addr - CHUNK_INFO_SIZE), &new_free, CHUNK_INFO_SIZE);
        memmove(new_chunk.addr, h->regions[i]->chunks, (j) * CHUNK_ARR);
        memcpy(((char *)new_chunk.addr - CHUNK_INFO_SIZE), &new_chunk, CHUNK_INFO_SIZE);
        h->regions[i]->chunks = (smart_ptr **)new_chunk.addr;
        h->regions[i]->chunks[0] = (smart_ptr *)((char *)new_chunk.addr - CHUNK_INFO_SIZE);
        h->regions[i]->chunks[j] = (smart_ptr *)((char *)new_free.addr - CHUNK_INFO_SIZE);

        h->regions[i]->n_chunks--;
        h->regions[i]->chunks[h->regions[i]->n_chunks-1]->size += CHUNK_ARR;
    }

    return;
}

void *change(heap_t *h, void *ptr, int size)
{
    int found = 0;
    int i, j = 0;

    void *new_ptr = NULL;

    for (i = 1; i < h->n_regions; i++) {
        for (j = 0; j < h->regions[i]->n_chunks; j++) {
            if (h->regions[i]->chunks[j]->addr == ptr) {
                found = 1;
                break;
            }
        }
        if (found == 1) {
            break;
        }
    }

    if (found == 0) {
        return NULL;
    }

    new_ptr = alloc(h, size);
    VALID(new_ptr, MEM_CODE, ALLOCATION_ERROR);

    if (size > h->regions[i]->chunks[j]->size) {
        memcpy(new_ptr, ptr, h->regions[i]->chunks[j]->size);
    } else {
        memcpy(new_ptr, ptr, size);
    }

    cull(h, ptr);

exit:
    return new_ptr;
}
