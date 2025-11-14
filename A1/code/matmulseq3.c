/***************************************************************************
 *
 * Parallelized Matrix-Matrix multiplication
 * init_matrix() and matmul_seq() now launch 1 thread per row
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 2048

static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

/* ------------ THREAD FUNCTION: initialize one row of A and B ------- */
void *
compute_init_row(void *arg)
{
    int i = *(int *)arg;
    int j;

    for (j = 0; j < SIZE; j++) {
        a[i][j] = 1.0;
        b[i][j] = 1.0;
    }

    return NULL;
}

/* ------------ init_matrix() now parallel --------------------------- */
static void
init_matrix(void)
{
    pthread_t threads[SIZE];
    int row_index[SIZE];
    int i;

    for (i = 0; i < SIZE; i++) {
        row_index[i] = i;
        pthread_create(&threads[i], NULL, compute_init_row, &row_index[i]);
    }

    for (i = 0; i < SIZE; i++)
        pthread_join(threads[i], NULL);
}

/* ------------ THREAD FUNCTION: computes one row of C -------------- */
void *
compute_row(void *arg)
{
    int i = *(int *)arg;   // row index
    int j, k;

    for (j = 0; j < SIZE; j++) {
        double sum = 0.0;
        for (k = 0; k < SIZE; k++)
            sum += a[i][k] * b[k][j];
        c[i][j] = sum;
    }
    return NULL;
}

/* ------------ matmul_seq() now parallel using pthreads ------------ */
static void
matmul_seq()
{
    pthread_t threads[SIZE];
    int row_index[SIZE];
    int i;

    for (i = 0; i < SIZE; i++) {
        row_index[i] = i;
        pthread_create(&threads[i], NULL, compute_row, &row_index[i]);
    }

    for (i = 0; i < SIZE; i++)
        pthread_join(threads[i], NULL);
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
    init_matrix();   // now parallel
    matmul_seq();    // also parallel
    //print_matrix();
    return 0;
}
