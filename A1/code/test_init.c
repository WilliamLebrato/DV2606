#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define KILO (1024)
#define MEGA (1024*1024)
#define MAX_ITEMS (64*MEGA)

static int *v;

static void
init_array(void)
{
    int i;
    v = (int *) malloc(MAX_ITEMS*sizeof(int));
    for (i = 0; i < MAX_ITEMS; i++)
        v[i] = rand();
}

int
main(int argc, char **argv)
{
    struct timespec start, end;
    double elapsed;

    // Time initialization
    clock_gettime(CLOCK_MONOTONIC, &start);
    init_array();
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Initialization time: %.6f seconds\n", elapsed);

    free(v);
    return 0;
}
