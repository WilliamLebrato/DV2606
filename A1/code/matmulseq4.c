/***************************************************************************
 *
 * Parallelized Matrix-Matrix multiplication
 * matmul_seq() now uses 16 threads (each handles a block of rows)
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

static void
init_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
            a[i][j] = 1.0;
            b[i][j] = 1.0;
        }
}

/* Argument struct for each thread: which rows to compute */
typedef struct {
    int start_row;
    int end_row;  // exclusive
} thread_arg_t;

/* ------------ THREAD FUNCTION: computes a block of rows of C -------- */
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

/* ------------ matmul_seq() now parallel using 16 pthreads ----------- */
static void
matmul_seq()
{
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    int rows_per_thread = SIZE / NUM_THREADS;
    int remainder = SIZE % NUM_THREADS;  // in case SIZE not divisible

    int start = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        int extra = (t < remainder) ? 1 : 0;   // spread remainder
        int end = start + rows_per_thread + extra;

        args[t].start_row = start;
        args[t].end_row   = end;

        if (pthread_create(&threads[t], NULL, compute_row, &args[t]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        start = end;
    }

    for (int t = 0; t < NUM_THREADS; t++) {
        if (pthread_join(threads[t], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }
}

/* ------------------------------------------------------------------ */

static void
print_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++)
            printf(" %7.2f", c[i][j]);
        printf("\n");
    }
}

int
main(int argc, char **argv)
{
    init_matrix();
    matmul_seq();     // now runs with exactly 16 threads
    //print_matrix();
    return 0;
}
