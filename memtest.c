#define WINDOWS 0
#define LOGGING 1

#include "memory.h"

#include <time.h>

int main(int argc, char *argv[])
{
    int min_size = 0;
    int max_size = 0;

    float **f_arrays = NULL;
    int n_floats = 0;
    int *f_sizes = NULL;
    bool **b_arrays = NULL;
    int n_bools = 0;
    int *b_sizes = NULL;
    int **i_arrays = NULL;
    int n_ints = 0;
    int *i_sizes = NULL;

    if (argc < 7) {
	printf("Correct format:\n\t%s <n_floats> <n_bools> <n_ints> <min_size> <max_size> <n_mixups>\n", argv[0]);
    	return 0;
    }

    srand((unsigned int)time(NULL));

    n_floats = atoi(argv[1]);
    n_bools = atoi(argv[2]);
    n_ints = atoi(argv[3]);
    min_size = atoi(argv[4]);
    max_size = atoi(argv[5]);

    new(f_arrays, n_floats, float *);
    new(b_arrays, n_bools, bool *);
    new(i_arrays, n_ints, int *);
    new(f_sizes, n_floats, int);
    new(b_sizes, n_bools, int);
    new(i_sizes, n_ints, int);

    for (int i = 0; i < n_floats; i++) {
	    f_sizes[i] = (int)(((float)rand()/(float)RAND_MAX) * (float)(max_size - min_size) + (float)(min_size));
	    new(f_arrays[i], f_sizes[i], float);
    }
    for (int i = 0; i < n_bools; i++) {
	    b_sizes[i] = (int)(((float)rand()/(float)RAND_MAX) * (float)(max_size - min_size) + (float)(min_size));
	    new(b_arrays[i], b_sizes[i], bool);
    }
    for (int i = 0; i < n_ints; i++) {
	    i_sizes[i] = (int)(((float)rand()/(float)RAND_MAX) * (float)(max_size - min_size) + (float)(min_size));
	    new(i_arrays[i], i_sizes[i], int);
    }

    for (int i = 0; i < n_floats; i++) {
	del(f_arrays[i]);
    }
    for (int i = 0; i < n_bools; i++) {
	del(b_arrays[i]);
    }
    for (int i = 0; i < n_ints; i++)  {
	del(i_arrays[i]);
    }
    del(f_sizes);
    del(b_sizes);
    del(i_sizes);
    del(f_arrays);
    del(b_arrays);
    del(i_arrays);

exit:
    return ret;
}
