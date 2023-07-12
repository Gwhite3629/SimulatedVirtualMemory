#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define UNALLOCATED 0
#define RESERVED 1
#define CULLED 2
#define AVAILABLE 3

extern const char *stat_names[12];

typedef struct __attribute__((packed)) {
    int size;
    int flag;
    char name[14];
    void *base;
    void *addr;
} smart_ptr;

typedef struct __attribute__((packed)) {
    int alloc_size;
    int used_size;
    int n_chunks;
    char name[10];
    void *base_addr;
    smart_ptr **chunks;
} region_t;

typedef struct __attribute__((packed)) {
    int alignment;
    int n_regions;
    char name[15];
    region_t **regions;
} heap_t;

extern heap_t *global_heap;
extern FILE *log_file;
extern int ret;

#define ALIGN 4096

#define CHUNK_INFO_SIZE sizeof(smart_ptr)
#define REGION_INFO_SIZE sizeof(region_t)
#define HEAP_INFO_SIZE sizeof(heap_t)

#define REGION_ARR sizeof(region_t *)
#define CHUNK_ARR sizeof(smart_ptr *)

#define HEAP_BASE_OFFSET (REGION_INFO_SIZE + (2*CHUNK_INFO_SIZE))
#define NEW_REGION_SIZE (REGION_INFO_SIZE + 3*CHUNK_INFO_SIZE + 3*CHUNK_ARR)

#define F_CHECK(F_CHUNK) \
    ((F_CHUNK->flag & 1) \
    ^ (F_CHUNK->flag & 2))

#define new(ptr, size, type, varname) \
    ptr = (type *)alloc(global_heap, size*sizeof(type), varname); \
    VALID(ptr, MEM_CODE, ALLOCATION_ERROR);

#define shorten(ptr, size, type) \
    (type *)shrink(global_heap, ptr, size*sizeof(type)); \
    VALID(ptr, MEM_CODE, ALLOCATION_ERROR);

#define widen(ptr, size, type) \
    (type *)expand(global_heap, ptr, size*sizeof(type), 0); \
    VALID(ptr, MEM_CODE, ALLOCATION_ERROR);

#define del(ptr) \
    cull(global_heap, ptr); \
    VALID(global_heap, MEM_CODE, ALLOCATION_ERROR);

#define log(h) \
    fprintf(log_file, "--------------------------------------------------------------------------------\n");\
    fprintf(log_file, "Heap: %16s\n", h->name); \
    for (int iter = 0; iter < h->n_regions; iter++) { \
        fprintf(log_file, "\nRegion: %12s // %16p : %d : %d : %d\n\n",\
        h->regions[iter]->name,\
        h->regions[iter]->base_addr,\
        h->regions[iter]->alloc_size,\
        h->regions[iter]->used_size,\
        h->regions[iter]->n_chunks);\
        for (int it = 0; it < h->regions[iter]->n_chunks; it++) {\
            fprintf(log_file, "\t%16s // %16s @ %p : %d\n",\
            h->regions[iter]->chunks[it]->name,\
            stat_names[h->regions[iter]->chunks[it]->flag],\
            h->regions[iter]->chunks[it]->addr,\
            h->regions[iter]->chunks[it]->size);\
        } \
    } \
    fprintf(log_file, "--------------------------------------------------------------------------------\n");\
    fprintf(log_file, "\n");

heap_t *create(int alignment, int size, const char *name);

heap_t *grow_kheap(heap_t *h);

void create_region(heap_t *h, int size);

void destroy(heap_t *h);

void *alloc(heap_t *h, int n, const char *name);

void cull(heap_t *h, void *ptr);

void clean(heap_t *h);

void r_print(region_t *r);

void h_print(heap_t *h);

#endif // _MEMORY_H_
