/***************************************************************************
 *
 * Parallelized Matrix-Matrix multiplication
 * init_matrix() and matmul_seq() now use 16 threads (row blocks)
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 2048
#define NUM_THREADS 16

static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

/* Thread argument for row-range processing */
typedef struct {
    int start_row;
    int end_row;  // exclusive
} thread_arg_t;

/* ------------ THREAD FUNCTION: initialize a block of rows -------- */
void *
compute_init_row(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    int i, j;

    for (i = targ->start_row; i < targ->end_row; i++) {
        for (j = 0; j < SIZE; j++) {
            a[i][j] = 1.0;
            b[i][j] = 1.0;
        }
    }

    return NULL;
}

/* ------------ init_matrix() now uses 16 threads ------------------- */
static void
init_matrix(void)
{
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    int rows_per_thread = SIZE / NUM_THREADS;
    int remainder = SIZE % NUM_THREADS;

    int start = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        int extra = (t < remainder) ? 1 : 0;
        int end = start + rows_per_thread + extra;

        args[t].start_row = start;
        args[t].end_row = end;

        pthread_create(&threads[t], NULL, compute_init_row, &args[t]);

        start = end;
    }

    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(threads[t], NULL);
}

/* ------------ THREAD FUNCTION: compute a block of rows of C ------- */
void *
compute_row(void *arg)
{
    thread_arg_t *targ = (thread_arg_t *)arg;
    int i, j, k;

    for (i = targ->start_row; i < targ->end_row; i++) {
        for (j = 0; j < SIZE; j++) {
            double sum = 0.0;
            for (k = 0; k < SIZE; k++)
                sum += a[i][k] * b[k][j];
            c[i][j] = sum;
        }
    }
    return NULL;
}

/* ------------ matmul_seq() now uses 16 threads --------------------- */
static void
matmul_seq()
{
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    int rows_per_thread = SIZE / NUM_THREADS;
    int remainder = SIZE % NUM_THREADS;

    int start = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        int extra = (t < remainder) ? 1 : 0;
        int end = start + rows_per_thread + extra;

        args[t].start_row = start;
        args[t].end_row = end;

        pthread_create(&threads[t], NULL, compute_row, &args[t]);

        start = end;
    }

    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(threads[t], NULL);
}

/* ------------------------------------------------------------------ */

static void
print_matrix(void)
{
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            printf(" %7.2f", c[i][j]);
        printf("\n");
    }
}

int
main(int argc, char **argv)
{
    init_matrix();   // now parallel with 16 threads
    matmul_seq();    // also parallel with 16 threads
    //print_matrix();
    return 0;
}
