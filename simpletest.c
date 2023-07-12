#include "memory.h"
#include "utils.h"

int main(void)
{
	int *usr_ptr_1 = NULL;
	char *usr_ptr_2 = NULL;
	double *usr_ptr_3 = NULL;
	float *usr_ptr_4 = NULL;


	new(usr_ptr_1, 128, int, "usr_ptr_1");

	new(usr_ptr_2, 5000, char, "usr_ptr_2");

	new(usr_ptr_3, 128, double, "usr_ptr_3");

	new(usr_ptr_4, 128, float, "usr_ptr_4");

	del(usr_ptr_1);
	del(usr_ptr_4);
	del(usr_ptr_3);
	del(usr_ptr_2);

exit:
	return 0;
}
