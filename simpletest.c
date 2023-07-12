#include "memory.h"
#include "utils.h"

int main(void)
{
	void *usr_ptr_1 = NULL;
	void *usr_ptr_2 = NULL;
	void *usr_ptr_3 = NULL;
	void *usr_ptr_4 = NULL;

	printf("Allocing 1\n");
	usr_ptr_1 = alloc(global_heap, 128, "usr_ptr_1");
	VALID(usr_ptr_1, MEM_CODE, ALLOCATION_ERROR);

	printf("Allocing 3\n");
	usr_ptr_3 = alloc(global_heap, 128, "usr_ptr_3");
	VALID(usr_ptr_3, MEM_CODE, ALLOCATION_ERROR);

	printf("Allocing 4\n");
	usr_ptr_4 = alloc(global_heap, 128, "usr_ptr_4");
	VALID(usr_ptr_4, MEM_CODE, ALLOCATION_ERROR);

	printf("Growing KHeap\n");
	global_heap = grow_kheap(global_heap);
	VALID(global_heap, MEM_CODE, ALLOCATION_ERROR);


	printf("Culling 1\n");
	cull(global_heap, usr_ptr_1);
	printf("Culling 4\n");
	cull(global_heap, usr_ptr_4);
	printf("Culling 3\n");
	cull(global_heap, usr_ptr_3);

	printf("Allocing 2\n");
	usr_ptr_2 = alloc(global_heap, 5000, "usr_ptr_2");
	VALID(usr_ptr_2, MEM_CODE, ALLOCATION_ERROR);

	printf("Culling 2\n");
	cull(global_heap, usr_ptr_2);

exit:
	return 0;
}
