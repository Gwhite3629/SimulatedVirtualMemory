#include "memory.h"

int main(void)
{
	global_heap = grow_kheap(global_heap);
	VALID(global_heap, MEM_CODE, ALLOCATION_ERROR);
exit:
	return 0;
}
