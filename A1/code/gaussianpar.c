/***************************************************************************
 *
 * Sequential version of Gaussian elimination
 *
 ***************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>   // atoi, exit, rand
#include <string.h>   // strcmp
#include <stdbool.h>


#define MAX_SIZE 4096
#define NUM_THREADS 15

typedef double matrix[MAX_SIZE][MAX_SIZE];

typedef struct gaussian_task {
    int k;
    int thread_id;
    struct gaussian_task* next;
} gaussian_task;

typedef struct {
    pthread_t threads[NUM_THREADS];
    gaussian_task* task_queue_head;
    gaussian_task* task_queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t complete_cond;
    int active_tasks;
    int total_tasks;
    bool shutdown;
} ThreadPool;

static ThreadPool pool;


int	N;		/* matrix size		*/
int	maxnum;		/* max number of element*/
char	*Init;		/* matrix init type	*/
int	PRINT;		/* print switch		*/
matrix	A;		/* matrix A		*/
double	b[MAX_SIZE];	/* vector b             */
double	y[MAX_SIZE];	/* vector y             */

/* forward declarations */
void work(void);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);
static void init_thread_pool(void);
static void shutdown_thread_pool(void);
static void wait_for_completion(void);

int
main(int argc, char **argv)
{
    int i, timestart, timeend, iter;

    Init_Default();		/* Init default values	*/
    Read_Options(argc,argv);	/* Read arguments	*/
    Init_Matrix();		/* Init the matrix	*/
    init_thread_pool();
    work();
    shutdown_thread_pool();
    if (PRINT == 1)
	   Print_Matrix();
}

static void enqueue_task(gaussian_task* task)
{
    pthread_mutex_lock(&pool.queue_mutex);

    task->next = NULL;
    if (pool.task_queue_tail == NULL) {
        pool.task_queue_head = task;
        pool.task_queue_tail = task;
    } else {
        pool.task_queue_tail->next = task;
        pool.task_queue_tail = task;
    }

    pool.total_tasks++;

    pthread_cond_signal(&pool.queue_cond);
    pthread_mutex_unlock(&pool.queue_mutex);
}

static gaussian_task* dequeue_task(void)
{
    gaussian_task* task = NULL;

    pthread_mutex_lock(&pool.queue_mutex);

    while (pool.task_queue_head == NULL && !pool.shutdown) {
        pthread_cond_wait(&pool.queue_cond, &pool.queue_mutex);
    }

    if (pool.task_queue_head != NULL) {
        task = pool.task_queue_head;
        pool.task_queue_head = task->next;
        if (pool.task_queue_head == NULL) {
            pool.task_queue_tail = NULL;
        }
        pool.active_tasks++;
    }

    pthread_mutex_unlock(&pool.queue_mutex);

    return task;
}

static void task_completed(void)
{
    pthread_mutex_lock(&pool.queue_mutex);
    pool.active_tasks--;
    pool.total_tasks--;

    if (pool.active_tasks == 0 && pool.total_tasks == 0) {
        pthread_cond_signal(&pool.complete_cond);
    }

    pthread_mutex_unlock(&pool.queue_mutex);
}

static void* worker_thread(void* arg)
{
    (void)arg;

    while (1) {
        gaussian_task* task = dequeue_task();

        if (task == NULL) {
            break;
        }

        int k = task->k;
        int thread_id = task->thread_id;
        int i, j;
        for (i = k + 1 + thread_id; i < N; i += NUM_THREADS) {
            for (j = k + 1; j < N; j++) {
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
            }
            b[i] = b[i] - A[i][k] * y[k];
            A[i][k] = 0.0;
        }

        free(task);
        task_completed();
    }

    return NULL;
}

static void init_thread_pool(void)
{
    pool.task_queue_head = NULL;
    pool.task_queue_tail = NULL;
    pool.active_tasks = 0;
    pool.total_tasks = 0;
    pool.shutdown = false;

    pthread_mutex_init(&pool.queue_mutex, NULL);
    pthread_cond_init(&pool.queue_cond, NULL);
    pthread_cond_init(&pool.complete_cond, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&pool.threads[i], NULL, worker_thread, NULL);
    }
}

static void shutdown_thread_pool(void)
{
    pthread_mutex_lock(&pool.queue_mutex);
    pool.shutdown = true;
    pthread_cond_broadcast(&pool.queue_cond);
    pthread_mutex_unlock(&pool.queue_mutex);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    pthread_mutex_destroy(&pool.queue_mutex);
    pthread_cond_destroy(&pool.queue_cond);
    pthread_cond_destroy(&pool.complete_cond);
}

static void wait_for_completion(void)
{
    pthread_mutex_lock(&pool.queue_mutex);

    while (pool.active_tasks > 0 || pool.total_tasks > 0) {
        pthread_cond_wait(&pool.complete_cond, &pool.queue_mutex);
    }

    pthread_mutex_unlock(&pool.queue_mutex);
}

void
work(void)
{
    int i, j, k;

    /* Gaussian elimination algorithm, Algo 8.4 from Grama */
    for (k = 0; k < N; k++) { /* Outer loop */
	    for (j = k+1; j < N; j++)
	       A[k][j] = A[k][j] / A[k][k]; /* Division step */
	    y[k] = b[k] / A[k][k];
	    A[k][k] = 1.0;
        // Parallel Threads, eliminating multiple rows each
        for (i = 0; i < NUM_THREADS; i++) {
            gaussian_task* task = malloc(sizeof(gaussian_task));
            task->k = k;
            task->thread_id = i;
            enqueue_task(task);
        }
        wait_for_completion();
    }
}

void
Init_Matrix()
{
    int i, j;

    printf("\nsize      = %dx%d ", N, N);
    printf("\nmaxnum    = %d \n", maxnum);
    printf("Init	  = %s \n", Init);
    printf("Initializing matrix...");

    if (strcmp(Init,"rand") == 0) {
        for (i = 0; i < N; i++){
            for (j = 0; j < N; j++) {
                if (i == j) /* diagonal dominance */
                    A[i][j] = (double)(rand() % maxnum) + 5.0;
                else
                    A[i][j] = (double)(rand() % maxnum) + 1.0;
            }
        }
    }
    if (strcmp(Init,"fast") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) /* diagonal dominance */
                    A[i][j] = 5.0;
                else
                    A[i][j] = 2.0;
            }
        }
    }

    /* Initialize vectors b and y */
    for (i = 0; i < N; i++) {
        b[i] = 2.0;
        y[i] = 1.0;
    }

    printf("done \n\n");
    if (PRINT == 1)
        Print_Matrix();
}

void
Print_Matrix()
{
    int i, j;

    printf("Matrix A:\n");
    for (i = 0; i < N; i++) {
        printf("[");
        for (j = 0; j < N; j++)
            printf(" %5.2f,", A[i][j]);
        printf("]\n");
    }
    printf("Vector b:\n[");
    for (j = 0; j < N; j++)
        printf(" %5.2f,", b[j]);
    printf("]\n");
    printf("Vector y:\n[");
    for (j = 0; j < N; j++)
        printf(" %5.2f,", y[j]);
    printf("]\n");
    printf("\n\n");
}

void
Init_Default()
{
    N = 2048;
    Init = "rand";
    maxnum = 15.0;
    PRINT = 0;
}

int
Read_Options(int argc, char **argv)
{
    char    *prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch ( *++*argv ) {
                case 'n':
                    --argc;
                    N = atoi(*++argv);
                    break;
                case 'h':
                    printf("\nHELP: try sor -u \n\n");
                    exit(0);
                    break;
                case 'u':
                    printf("\nUsage: gaussian [-n problemsize]\n");
                    printf("           [-D] show default values \n");
                    printf("           [-h] help \n");
                    printf("           [-I init_type] fast/rand \n");
                    printf("           [-m maxnum] max random no \n");
                    printf("           [-P print_switch] 0/1 \n");
                    exit(0);
                    break;
                case 'D':
                    printf("\nDefault:  n         = %d ", N);
                    printf("\n          Init      = rand" );
                    printf("\n          maxnum    = 5 ");
                    printf("\n          P         = 0 \n\n");
                    exit(0);
                    break;
                case 'I':
                    --argc;
                    Init = *++argv;
                    break;
                case 'm':
                    --argc;
                    maxnum = atoi(*++argv);
                    break;
                case 'P':
                    --argc;
                    PRINT = atoi(*++argv);
                    break;
                default:
                    printf("%s: ignored option: -%s\n", prog, *argv);
                    printf("HELP: try %s -u \n\n", prog);
                    break;
            }
}
