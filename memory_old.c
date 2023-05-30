#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

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

/*
__attribute__ ((destructor)) static inline void end(void)
{
	destroy(global_heap);
}
*/

inline void r_print(region_t *r)
{
    printf("%p : %d : %d : %d\n", r->base_addr, r->alloc_size, r->used_size, r->n_chunks);
    for (int i = 0; i < r->n_chunks; i++) {
        printf("\t%12s @ %p/%p : %d\n", 
        stat_names[r->chunks[i].flag], 
        r->chunks[i].base,
        r->chunks[i].addr, 
        r->chunks[i].size);
    }
}

inline region_t* create_region(heap_t *h, int alignment, int size)
{
    region_t r;
    void *temp_base = NULL;
    smart_ptr temp_ptr_chunks;
    smart_ptr temp_ptr_region;
    smart_ptr temp_ptr_free;

    int align_size = ((3*sizeof(smart_ptr) + sizeof(region_t) + size) / h->regions[0]->alignment) + 1;

    #if WINDOWS
        temp_base = (void *)_aligned_malloc(align_size*alignment, alignment);
	    VALID(temp_base, MEM_CODE, ALLOCATION_ERROR);
	    memset(temp_base, 0, align_size*alignment);
    #else
	    temp_base = (void *)aligned_alloc(alignment, align_size*alignment);
	    VALID(temp_base, MEM_CODE, ALLOCATION_ERROR); 
     	memset(temp_base, 0, align_size*alignment);	
    #endif


	temp_ptr_chunks.addr = temp_base;
	temp_ptr_chunks.base = temp_base;
	temp_ptr_chunks.flag = RESERVED;
	temp_ptr_chunks.size = 3*sizeof(smart_ptr);

	temp_ptr_region.addr = ((char *)temp_ptr_chunks.addr + 3*sizeof(smart_ptr));
	temp_ptr_region.base = temp_base;
	temp_ptr_region.flag = RESERVED;
	temp_ptr_region.size = sizeof(region_t);

	temp_ptr_free.addr = ((char *)temp_ptr_region.addr + sizeof(region_t));
	temp_ptr_free.base = temp_base;
	temp_ptr_free.flag = UNALLOCATED;
	temp_ptr_free.size = ((align_size*alignment) - (sizeof(region_t) + 3*sizeof(smart_ptr)));

	r.alignment = alignment;
	r.alloc_size = align_size*alignment;
	r.base_addr = temp_base;
	r.n_chunks = 3;
	r.chunks = temp_ptr_chunks.addr;
	r.chunks[0] = temp_ptr_chunks;
	r.chunks[1] = temp_ptr_region;
	r.chunks[2] = temp_ptr_free;
	r.used_size = 3*sizeof(smart_ptr) + sizeof(region_t);

    (*((region_t *)r.chunks[1].addr)) = r;

exit:
    return r.chunks[1].addr;
}

inline void *alloc_region(heap_t *h, region_t *r, int n)
{
    void *ptr = NULL;
    int found = 1;

    for (int i = 0; i < r->n_chunks; i++) {
        if (!((r->chunks[i].flag & 1) ^ \
              (r->chunks[i].flag & 2)) & \
              (r->chunks[i].size >= n)) {
            if (r->chunks[i].size == n) {
                r->chunks[i].flag = RESERVED;
                ptr = r->chunks[i].addr;
		        printf("Early Find\n");
		        break;
            } else {
                printf("Late find\n");
                r->chunks = (smart_ptr *)expand(h, r->chunks, (r->n_chunks+1)*sizeof(smart_ptr), 0);
                VALID(r->chunks, MEM_CODE, ALLOCATION_ERROR);
		        for (int j = r->n_chunks; j > i; j--) {
                    r->chunks[j] = r->chunks[j-1];
                }
                r->chunks[i+1].size = r->chunks[i].size - n;
                r->chunks[i+1].flag = UNALLOCATED;
                r->chunks[i+1].addr = ((char *)r->chunks[i].addr + n);
                r->chunks[i].size = n;
                r->chunks[i].flag = RESERVED;
                ptr = r->chunks[i].addr;
                r->n_chunks++;
                break;
            }
        }
    }

    r->used_size += n;

    return ptr;

exit:
    return NULL;
}

inline region_t *expand_chunks(heap_t *h, region_t *r, int n_chunks) {

}

inline region_t *expand_region(heap_t *h, region_t *r, int ptr_idx, int old_size, int addition)
{

    if (ptr_idx != (r->n_chunks - 1)) {
        if (!((r->chunks[ptr_idx+1].flag & 1) ^ \
              (r->chunks[ptr_idx+1].flag & 2)) & \
              (r->chunks[ptr_idx+1].size >= (addition))) {
            if (r->chunks[ptr_idx+1].size == addition) {
                for (int i = ptr_idx + 1; i < r->n_chunks-1; i++) {
                    r->chunks[i] = r->chunks[i+1];
                }
                r->chunks = (smart_ptr *)expand(h, r->chunks, (r->n_chunks+1)*sizeof(smart_ptr), 1);
		        VALID(r->chunks, MEM_CODE, ALLOCATION_ERROR);

                r->chunks[ptr_idx].size += addition;

                r->used_size += addition;

                r->n_chunks--;
            } else {
                r->chunks[ptr_idx+1].addr = ((char *)r->chunks[ptr_idx+1].addr + addition);
                r->chunks[ptr_idx+1].size -= addition;
                r->chunks[ptr_idx].size += addition;
                r->used_size += addition;
            }
        }
        return r;
    }

    for (int i = 0; i < r->n_chunks; i++) {
        if (!((r->chunks[i].flag & 1) ^ \
              (r->chunks[i].flag & 2)) & \
              (r->chunks[i].size >= (old_size + addition))) {
            r->chunks = (smart_ptr *)expand(h, r->chunks, (r->n_chunks+1)*sizeof(smart_ptr), 1);
            VALID(r->chunks, MEM_CODE, ALLOCATION_ERROR);

	        for (int j = r->n_chunks; j > i; j--) {
                r->chunks[j-1] = r->chunks[j];
            }
            r->chunks[i].flag = UNALLOCATED;
            r->chunks[i].size -= (old_size + addition);
            r->chunks[i].addr = ((char *)r->chunks[i].addr + old_size + addition);
            r->chunks[ptr_idx].flag = RESERVED;
            r->chunks[ptr_idx].size = old_size + addition;
            r->n_chunks++;
            break;
        }
    }

    r->used_size += addition;

    return r;

exit:
    return NULL;
}

inline region_t *cull_region(region_t *r, void *ptr, int ptr_idx, int n)
{
    if (ptr == NULL) {
        goto exit;
    }

    r->chunks[ptr_idx].flag = CULLED;

    r->used_size -= n;

    return r;

exit:
    return NULL;
}

inline region_t *clean_region(heap_t *h, region_t *r)
{
    int done = 1;
    while (done == 1) {
        done = 0;
        for (int i = 0; i < r->n_chunks-1; i++) {
            if (r->chunks[i].flag == CULLED) {
                memset(r->chunks[i].addr, 0, r->chunks[i].size);
                r->chunks[i].flag = AVAILABLE;
            }
            if (r->chunks[i].flag != RESERVED) {
                if (r->chunks[i + 1].flag != RESERVED) {
                    r->chunks[i].size += r->chunks[i+1].size;
                    for (int j = i + 1; j < r->n_chunks-1; j++) {
                        r->chunks[j] = r->chunks[j+1];
                    }
                    r->chunks = (smart_ptr *)shrink(h, r->chunks, (r->n_chunks-1)*sizeof(smart_ptr));
                    VALID(r->chunks, MEM_CODE, ALLOCATION_ERROR);
		    
		            r->n_chunks--;
                    done = 1;
                }
            }
        }
    }

    return r;

exit:
    return NULL;
}

inline void destroy_region(region_t *r)
{
    if (r) {
        #if WINDOWS
            if (r->base_addr)
                _aligned_free(r->base_addr);
            r->base_addr = NULL;
        #else
            if (r->base_addr)
                free(r->base_addr);
            r->base_addr = NULL;
        #endif
    }
}

inline void h_print(heap_t *h)
{
    printf("Heap: %s\n", h->name);
    for (int i = 0; i < h->n_regions; i++) {
        r_print(h->regions[i]);
    }
    printf("\n");
}

inline heap_t *create(int alignment, int size, const char *name)
{
    heap_t *h = NULL;
    void *temp_base = NULL;
    smart_ptr temp_ptr_chunks;
    smart_ptr temp_ptr_region;
    smart_ptr temp_ptr_region_array;
    smart_ptr temp_ptr_heap;
    smart_ptr temp_ptr_free;
    region_t temp_region;

    #if WINDOWS
        temp_base = (void *)_aligned_malloc(size, alignment);
        VALID(temp_base, MEM_CODE, ALLOCATION_ERROR);
	    memset(temp_base, 0, size);
    #else
        temp_base = (void *)aligned_alloc(alignment, size);
        VALID(temp_base, MEM_CODE, ALLOCATION_ERROR);
	    memset(temp_base, 0, size);
    #endif

    temp_ptr_chunks.addr = temp_base;
    temp_ptr_chunks.base = temp_base;
    temp_ptr_chunks.flag = RESERVED;
    temp_ptr_chunks.size = 5*sizeof(smart_ptr);

    temp_ptr_region.addr = ((char *)temp_ptr_chunks.addr + 5*sizeof(smart_ptr));
    temp_ptr_region.base = temp_base;
    temp_ptr_region.flag = RESERVED;
    temp_ptr_region.size = sizeof(region_t);

    temp_ptr_region_array.addr = ((char *)temp_ptr_region.addr + sizeof(region_t));
    temp_ptr_region_array.base = temp_base;
    temp_ptr_region_array.flag = RESERVED;
    temp_ptr_region_array.size = sizeof(region_t *);

    temp_ptr_heap.addr = ((char *)temp_ptr_region_array.addr + sizeof(region_t *));
    temp_ptr_heap.base = temp_base;
    temp_ptr_heap.flag = RESERVED;
    temp_ptr_heap.size = sizeof(heap_t);

    temp_ptr_free.addr = ((char *)temp_ptr_heap.addr + sizeof(heap_t));
    temp_ptr_free.base = temp_base;
    temp_ptr_free.flag = UNALLOCATED;
    temp_ptr_free.size = (size - (5*sizeof(smart_ptr)+sizeof(region_t)+sizeof(region_t *)+sizeof(heap_t)));

    temp_region.alignment = alignment;
    temp_region.alloc_size = size;
    temp_region.base_addr = (char *)temp_base;
    temp_region.n_chunks = 5;
    temp_region.chunks = temp_ptr_chunks.addr;
    temp_region.chunks[0] = temp_ptr_chunks;
    temp_region.chunks[1] = temp_ptr_region;
    temp_region.chunks[2] = temp_ptr_region_array;
    temp_region.chunks[3] = temp_ptr_heap;
    temp_region.chunks[4] = temp_ptr_free;
    temp_region.used_size = 5*sizeof(smart_ptr)+sizeof(region_t)+sizeof(region_t *)+sizeof(heap_t);

    h = temp_ptr_heap.addr;

    (*((region_t *)temp_region.chunks[1].addr)) = temp_region;

    h->regions = temp_ptr_region_array.addr;
    h->regions[0] = temp_region.chunks[1].addr;

    strcpy(h->name, name);

    h->n_regions = 1;

    #if LOGGING
        log_file = fopen("mem_log.txt", "w+");
	VALID(log_file, FILE_CODE, FILE_OPEN);    
    #endif

    #if LOGGING
        log(h);
    #endif

    return h;

exit:
    return NULL;
}

inline heap_t *grow(heap_t *h, int n)
{
    smart_ptr temp_chunk;
    int al = h->regions[0]->alignment;
    region_t *temp = create_region(h, al, n);
    VALID(temp, MEM_CODE, ALLOCATION_ERROR);
    temp_chunk.addr = ((char *)temp->chunks[2].addr + (h->n_regions + 1)*sizeof(region_t *));
    temp_chunk.base = temp->base_addr;
    temp_chunk.flag = UNALLOCATED;
    temp_chunk.size = (temp->chunks[2].size - (h->n_regions + 1)*sizeof(region_t *));
    temp->chunks[2].flag = RESERVED;
    temp->chunks[2].size = (h->n_regions + 1)*sizeof(region_t *);
    temp->n_chunks++;
    temp->chunks[3] = temp_chunk;
    region_t **temp_array = temp->chunks[2].addr;
    memcpy(temp_array, h->regions, h->n_regions*sizeof(region_t *));
    h->regions = temp_array;
    h->regions[h->n_regions] = temp;
    h->n_regions++;

    #if LOGGING
        log(h);
    #endif

    return h;

exit:
    return NULL;
}

inline void *alloc(heap_t *h, int n)
{
    int found = 0;
    void *ptr = NULL;

	int alloc_size = 0;
	int used_size = 0;
    printf("N: %d\n", h->n_regions);
    for (int i = 0; i < h->n_regions; i++) {
	    alloc_size = h->regions[i]->alloc_size;
	    used_size = h->regions[i]->used_size;
        if (n < (alloc_size - used_size)) {
            for (int j = 0; j < h->regions[i]->n_chunks; j++) {
                if (!((h->regions[i]->chunks[j].flag & 1) ^ \
                      (h->regions[i]->chunks[j].flag & 2)) & \
                      (h->regions[i]->chunks[j].size >= (n))) { 
			        ptr = alloc_region(h, h->regions[i], n);
		     	    found = 1;
                    break;
                }
            }
        }
        if (found == 1)
            break;
    }

    if (found == 0) {
	    h = grow(h, (n / h->regions[0]->alignment) + 1);
        ptr = alloc_region(h, h->regions[(h->n_regions-1)], n);
    }

    #if LOGGING
        log(h);
    #endif
/*
    fprintf(log_file, "HeapL %s\n", h->name);
    for (int iter = 0; iter < h->n_regions; iter++) {
	fprintf(log_file, "%p :", h->regions[iter].base_addr);
	fprintf(log_file, "%d :", h->regions[iter].alloc_size);
	fprintf(log_file, "%d :", h->regions[iter].used_size);
	fprintf(log_file, "%d\n", h->regions[iter].n_chunks);
	for (int it = 0; it < h->regions[iter].n_chunks; it++) {
		fprintf(log_file, "\t%12s @ %p/%p : %d\n",
		stat_names[h->regions[iter].chunks[it].flag],
		h->regions[iter].chunks[it].base,
		h->regions[iter].chunks[it].addr,
		h->regions[iter].chunks[it].size);
	}
    }
    fprintf(log_file, "\n\n");
*/
   
    return ptr;

}

inline void *expand(heap_t *h, void *ptr, int size, bool strict)
{
    int ptr_idx = -1;
    int ptr_reg = -1;
    int new_idx = -1;
    int found = 0;
    int i, j, k = 0;

    for (i = 0; i < h->n_regions; i++) {
        for (j = 0; j < h->regions[i]->n_chunks; j++) {
            if (h->regions[i]->chunks[j].addr == ptr) {
                ptr_idx = j;
                break;
            }
        }
        if (ptr_idx >= 0)
            break;
    }

    ptr_reg = i;

    if (strict) goto RECURSE;

    // Pointer index isn't the last one
	if (ptr_idx != (h->regions[i]->n_chunks - 1)) {
        // Next index is free and large enough
        if (!((h->regions[i]->chunks[ptr_idx+1].flag & 1) ^ \
            (h->regions[i]->chunks[ptr_idx+1].flag & 2)) & \
            (h->regions[i]->chunks[ptr_idx+1].size >= (size - h->regions[i]->chunks[ptr_idx].size))) {
            found = 1;
        // Last index is free and large enough
        } else if (!((h->regions[i]->chunks[h->regions[i]->n_chunks-1].flag & 1) ^ \
                    (h->regions[i]->chunks[h->regions[i]->n_chunks-1].flag & 2)) & \
                    (h->regions[i]->chunks[h->regions[i]->n_chunks-1].size >= (size))) {
            found = 1;
        }
	}
    if (found == 1) {
        h->regions[i] = expand_region(h, h->regions[i], ptr_idx, h->regions[i]->chunks[ptr_idx].size, size - h->regions[i]->chunks[ptr_idx].size);
        return ptr;
    } else {
        for (i = 0; i < h->n_regions; i++) {
            for (j = 0; j < h->regions[i]->n_chunks; j++) {
                if (!((h->regions[i]->chunks[j].flag & 1) ^ \
                      (h->regions[i]->chunks[j].flag & 2)) & \
                      (h->regions[i]->chunks[j].size >= (size))) {
                            found = 1;
                            break;
                }
            }
            if (found == 1)
                break;
        }
    }

    if (found == 1) {
        printf("Good find\n");  
        ptr = alloc(h, size);
        memcpy(ptr, h->regions[ptr_reg]->chunks[ptr_idx].addr, h->regions[ptr_reg]->chunks[ptr_idx].size);
        for (k = 0; k < h->n_regions; k++) {
            for (j = 0; j < h->regions[k]->n_chunks; j++) {
                if (h->regions[k]->chunks[j].addr == ptr) {
                    new_idx = j;
                    break;
                }
            }
            if (new_idx >= 0)
                break;
        }
        h->regions[k]->chunks[new_idx].base = h->regions[k]->base_addr;
        h->regions[k]->chunks[new_idx].size = size;
        h->regions[ptr_reg] = cull_region(h->regions[ptr_reg], h->regions[ptr_reg]->chunks[ptr_idx].addr, ptr_idx, h->regions[ptr_reg]->chunks[ptr_idx].size);
    } else {
RECURSE:
        printf("Grow\n");
        h = grow(h, (size / h->regions[0]->alignment) + 1);
        ptr = alloc(h, size);
        memcpy(ptr, h->regions[ptr_reg]->chunks[ptr_idx].addr, h->regions[ptr_reg]->chunks[ptr_idx].size);
        h->regions[ptr_reg]->chunks[h->n_regions-1].base = h->regions[ptr_reg]->base_addr;
        h->regions[ptr_reg]->chunks[h->n_regions-1].size = size;
        h->regions[ptr_reg] = cull_region(h->regions[ptr_reg], h->regions[ptr_reg]->chunks[ptr_idx].addr, ptr_idx, h->regions[ptr_reg]->chunks[ptr_idx].size);
    }

    #if LOGGING
        log(h);
    #endif

    return ptr;
}

inline void *shrink(heap_t *h, void *ptr, int size)
{
    int ptr_idx = -1;
    int i = 0, j = 0;
    int old_size = 0;

    if (size <= 0) {
        return NULL;
    }

    for (i = 0; i < h->n_regions; i++) {
        for (j = 0; j < h->regions[i]->n_chunks; j++) {
            if (h->regions[i]->chunks[j].addr == ptr) {
                ptr_idx = j;
                break;
            }
        }
        if (ptr_idx >= 0)
            break;
    }

    old_size = h->regions[i]->chunks[ptr_idx].size;

    if ((h->regions[i]->n_chunks - 1) == ptr_idx) {
        h->regions[i]->chunks = (smart_ptr *)expand(h, h->regions[i]->chunks, (h->regions[i]->n_chunks+1)*sizeof(smart_ptr), 0);
        VALID(h->regions[i]->chunks, MEM_CODE, ALLOCATION_ERROR);
	
	    h->regions[i]->chunks[h->regions[i]->n_chunks].size = h->regions[i]->chunks[ptr_idx].size - size;
        h->regions[i]->chunks[h->regions[i]->n_chunks].flag = CULLED;
        h->regions[i]->chunks[h->regions[i]->n_chunks].addr = ((char *)ptr + size);
        h->regions[i]->chunks[ptr_idx].size = size;
        h->regions[i]->n_chunks++;
    } else {
        if (!((h->regions[i]->chunks[ptr_idx+1].flag & 1) ^ \
              (h->regions[i]->chunks[ptr_idx+1].flag & 2))) {
            h->regions[i]->chunks[ptr_idx+1].addr = ((char *)h->regions[i]->chunks[ptr_idx+1].addr - size);
            h->regions[i]->chunks[ptr_idx+1].size += size;
            h->regions[i]->chunks[ptr_idx].size -= size;
        } else {
            h->regions[i]->chunks = (smart_ptr *)expand(h, h->regions[i]->chunks, (h->regions[i]->n_chunks+1)*sizeof(smart_ptr), 0);
            VALID(h->regions[i]->chunks, MEM_CODE, ALLOCATION_ERROR);
	    
	    for (j = h->regions[i]->n_chunks; j > ptr_idx; j--) {
                h->regions[i]->chunks[j] = h->regions[i]->chunks[j-1];
            }
            h->regions[i]->chunks[ptr_idx+1].size = h->regions[i]->chunks[ptr_idx].size - size;
            h->regions[i]->chunks[ptr_idx+1].flag = CULLED;
            h->regions[i]->chunks[ptr_idx+1].addr = ((char *)ptr + size);
            h->regions[i]->chunks[ptr_idx].size = size;
            h->regions[i]->n_chunks++;
        }
    }

    h->regions[i]->used_size -= (old_size - size);

    #if LOGGING
        log(h);
    #endif

    return ptr;

exit:
    return NULL;
}

inline heap_t *cull(heap_t *h, void *ptr)
{
    int found = 0;
    int ptr_idx = -1;
    int i, j = 0;

    for (i = 0; i < h->n_regions; i++) {
        for (j = 0; j < h->regions[i]->n_chunks; j++) {
            if (h->regions[i]->chunks[j].addr == ptr) {
                found = 1;
                ptr_idx = j;
                break;
            }
        }
        if (found == 1)
            break;
    }

    if (found == 1)
        h->regions[i] = cull_region(h->regions[i], ptr, ptr_idx, h->regions[i]->chunks[ptr_idx].size);

    #if LOGGING
        log(h);
    #endif

    return h;
}

inline heap_t *clean(heap_t *h)
{
    for (int i = 0; i < h->n_regions; i++) {
        clean_region(h, h->regions[i]);
    }

    #if LOGGING
        log(h);
    #endif

    return h;
}

inline void destroy(heap_t *h)
{
    if (h) {
        void *temp[h->n_regions];
        for (int i = 0; i < h->n_regions; i++) {
            printf("Access %d\n", i);
            temp[i] = h->regions[i] ? h->regions[i]->base_addr : 0;
        }
        int num = h->n_regions;
        for (int i = 0; i < num; i++) {
            for (int j = i; j < num; j++) {
                if (temp[i] == temp[j]) {
                    for (int k = j; k < num; k++) {
                        temp[k] = temp[k+1];
                    }
                    j--;
                    num--;
                }
            }
        }
        for (int i = 0; i < num; i++) {
            if (temp[i]) {
                free(temp[i]);
            }
        }
    }

    #if LOGGING
        fclose(log_file);
    #endif
}
