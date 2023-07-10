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
	global_heap = create(ALIGN, 1*ALIGN, "global");
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
inline heap_t *create(int alignment, int size, const char *name)
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
    strcpy(heap_reg_info.name, "KREG_PTR");
    heap_reg_info.base = base_kheap;
    heap_reg_info.addr = (char *)base_kheap + CHUNK_INFO_SIZE;

    // Smart ptr for heap info
    heap_ptr.size = HEAP_INFO_SIZE;
    heap_ptr.flag = RESERVED;
    strcpy(heap_ptr.name, "KHEAP_PTR");
    heap_ptr.base = base_kheap;
    heap_ptr.addr = (char *)heap_reg_info.addr +  REGION_INFO_SIZE + CHUNK_INFO_SIZE;

    // Smart ptr for region array
    reg_arr.size = 2 * REGION_ARR;
    reg_arr.flag = RESERVED;
    strcpy(reg_arr.name, "REG_ARR");
    reg_arr.base = base_kheap;
    reg_arr.addr = (char *)heap_ptr.addr + HEAP_INFO_SIZE + CHUNK_INFO_SIZE;
    
    // Smart ptr for chunk array
    heap_chunk_arr.size = 5 * CHUNK_ARR;
    heap_chunk_arr.flag = RESERVED;
    strcpy(heap_chunk_arr.name, "CHUNK_ARR");
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
    strcpy(kheap.name, "KHEAP");
    kheap.base_addr = base_kheap;
    kheap.chunks = heap_chunk_arr.addr;

    // Heap info
    h.alignment = alignment;
    h.n_regions = 2;
    strcpy(h.name, "GLOBAL");
    h.regions = reg_arr.addr;

    // Smart ptr for remaining free space
    heap_free_space.size = (size - kheap.used_size);
    heap_free_space.flag = UNALLOCATED;
    strcpy(heap_free_space.name, "KHEAP_FREE");
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
    strcpy(reg_info.name, "REG1_PTR");
    reg_info.base = base_reg1;
    reg_info.addr = (char *)base_reg1 + CHUNK_INFO_SIZE;

    // Smart ptr for chunk array
    chunk_arr.size = 3 * CHUNK_ARR;
    chunk_arr.flag = RESERVED;
    strcpy(chunk_arr.name, "CHUNK_ARR");
    chunk_arr.base = base_reg1;
    chunk_arr.addr = ((char *)base_reg1 + size) - chunk_arr.size;

    // Region info
    reg1.alloc_size =  size;
    reg1.used_size =  REGION_INFO_SIZE
                    +  3 * CHUNK_INFO_SIZE
                    +  3 * CHUNK_ARR; 
    reg1.n_chunks = 3;
    strcpy(reg1.name, "HEAP");
    reg1.base_addr = base_reg1;
    reg1.chunks = chunk_arr.addr;

    // Smart ptr for remaining free space
    free_space.size = (size - reg1.used_size);
    free_space.flag = UNALLOCATED;
    strcpy(free_space.name, "HEAP_FREE");
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

    #if LOGGING
        log_file = fopen("mem_log.txt", "w+");
	    VALID(log_file, FILE_CODE, FILE_OPEN);    
    #endif

    #if LOGGING
        log(((heap_t *)kheap.chunks[2]->addr));
    #endif

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

    #if LOGGING
        fclose(log_file);
    #endif
}

inline heap_t *grow_kheap(heap_t *h)
{
    void *temp;
    void *h_base = h->regions[0]->base_addr;
    int h_size = h->regions[0]->alloc_size;
    void *old_tail;
    smart_ptr *chunks[5];

    #if WINDOWS
        temp = (void *)_aligned_malloc(h_size + h->alignment, h->alignment);
        VALID(temp, MEM_CODE, ALLOCATION_ERROR);
	    memset(temp, 0, h_size + h->alignment);
    #else
        temp = (void *)aligned_alloc(h->alignment, h_size + h->alignment);
        VALID(temp, MEM_CODE, ALLOCATION_ERROR);
	    memset(temp, 0, h_size + h->alignment);
    #endif

    int MEM_OFFSET = (char *)temp - (char *)h->regions[0]->base_addr;

    h->regions[0]->base_addr = temp;

    heap_t temp_heap;
    temp_heap.alignment = h->alignment;
    temp_heap.n_regions = h->n_regions;
    strcpy(temp_heap.name, h->name);

    for (int i = 0; i < 5; i++)
    {
        smart_ptr t_ptr;
        t_ptr.addr = (char *)h->regions[0]->chunks[i]->addr + MEM_OFFSET;
        t_ptr.base = (char *)h->regions[0]->chunks[i]->base + MEM_OFFSET;
        strcpy(t_ptr.name, h->regions[0]->chunks[i]->name);
        t_ptr.size = h->regions[0]->chunks[i]->size;
        t_ptr.flag = h->regions[0]->chunks[i]->flag;
        h->regions[0]->chunks[i] = (smart_ptr *)((char *)h->regions[0]->chunks[i]->addr - CHUNK_INFO_SIZE);
        memcpy((smart_ptr *)((char *)h->regions[0]->chunks[i]), &t_ptr, CHUNK_INFO_SIZE);
        chunks[i] = (smart_ptr *)((char *)t_ptr.addr - CHUNK_INFO_SIZE);
    }
    region_t temp_region;
    temp_region.alloc_size = h->regions[0]->alloc_size + ALIGN;
    temp_region.base_addr = temp;
    temp_region.n_chunks = h->regions[0]->n_chunks;
    strcpy(temp_region.name, h->regions[0]->name);
    temp_region.used_size = h->regions[0]->used_size;
    temp_region.chunks = (smart_ptr **)((char *)h->regions[0]->chunks + MEM_OFFSET);
    h->regions = (region_t **)((char *)h->regions + MEM_OFFSET);
    h->regions[0] = (region_t *)h->regions;
    memcpy(h->regions[0], &temp_region, REGION_INFO_SIZE);
    temp_heap.regions = h->regions;

    memcpy(temp, h_base, h_size); // Copy all of kheap

    smart_ptr arr_ptr;

    arr_ptr.size = 5 * CHUNK_ARR;
    arr_ptr.flag = RESERVED;
    strcpy(arr_ptr.name, "CHUNK_ARR");
    arr_ptr.base = temp;
    arr_ptr.addr = ((char *)temp + (h_size + ALIGN)) - arr_ptr.size;

    temp_region.chunks = (smart_ptr **)((char *)temp + (h_size + ALIGN) - (5*CHUNK_ARR));
    h->regions[0]->chunks = (smart_ptr **)((char *)temp + (h_size + ALIGN) - (5*CHUNK_ARR));
    h->regions[0]->chunks[0] = (smart_ptr *)((char *)temp + (h_size + ALIGN) - (5*CHUNK_ARR + CHUNK_INFO_SIZE));
    h->regions[0]->chunks[1] = chunks[1];
    h->regions[0]->chunks[2] = chunks[2];
    h->regions[0]->chunks[3] = chunks[3];
    h->regions[0]->chunks[4] = chunks[4];
    h->regions[0]->chunks[4]->size += ALIGN;
    memcpy(((heap_t *)h->regions[0]->chunks[2]->addr)->regions[0], &temp_region, REGION_INFO_SIZE);
    memcpy(h->regions[0]->chunks[2]->addr, &temp_heap, HEAP_INFO_SIZE);
    
    memcpy(h->regions[0]->chunks[0],
            &arr_ptr,
            CHUNK_INFO_SIZE); // Copy chunk smart ptr

    memcpy(h->regions[0]->chunks[0], &arr_ptr, CHUNK_INFO_SIZE);

    // Clear old memory
    memset(((char *)h_base + h_size - (5*CHUNK_ARR + CHUNK_INFO_SIZE)), 0, 5*CHUNK_ARR + CHUNK_INFO_SIZE);

    h = (heap_t *)h->regions[0]->chunks[2]->addr;

    free(h_base);

    /*#if LOGGING
        log(h);
    #endif*/

    fprintf(log_file, "Heap: %12s\n", h->name);
    for (int iter = 0; iter < h->n_regions; iter++) {
        fprintf(log_file, "%12s // %p : %d : %d : %d\n", h->regions[iter]->name, h->regions[iter]->base_addr, h->regions[iter]->alloc_size, h->regions[iter]->used_size, h->regions[iter]->n_chunks);
        for (int it = 0; it < h->regions[iter]->n_chunks; it++) {
            fprintf(log_file, "\t%12s // %12s @ %p : %d\n", h->regions[iter]->chunks[it]->name, stat_names[h->regions[iter]->chunks[it]->flag], h->regions[iter]->chunks[it]->addr, h->regions[iter]->chunks[it]->size);
        }
    }
    fprintf(log_file, "\n\n");


    return h;

exit:
    return NULL;
}

/*
inline void *alloc(heap_t *h, int n, const char *name)
{

}
*/

/*
inline region_t* create_region(heap_t *h, int alignment, int size)
{

}
*/
