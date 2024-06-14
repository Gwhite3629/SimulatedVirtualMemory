#include "memory.h"
#include "utils.h"

int main(void)
{
	int *ints = NULL;
	char *chars = NULL;
	double *doubles = NULL;
	float *floats = NULL;


	new(ints, 128, int);

	new(chars, 5000, char);

	new(doubles, 128, double);

	new(floats, 128, float);

	alt(doubles, 256, double);

	alt(floats, 64, float);

	del(ints);
	del(floats);
	del(doubles);
	del(chars);

exit:
	return 0;
}
