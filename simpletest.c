#include "memory.h"
#include "utils.h"

int main(void)
{
	void *usr_ptr_1 = NULL;
	void *usr_ptr_2 = NULL;

	usr_ptr_1 = alloc(global_heap, 128, "usr_ptr_1");
	VALID(usr_ptr_1, MEM_CODE, ALLOCATION_ERROR);

	global_heap = grow_kheap(global_heap);
	VALID(global_heap, MEM_CODE, ALLOCATION_ERROR);

	usr_ptr_2 = alloc(global_heap, 5000, "usr_ptr_2");
	VALID(usr_ptr_2, MEM_CODE, ALLOCATION_ERROR);

exit:
	return 0;
}
