/***************************************************************************
 *
 * Parallel version of Quick sort using thread pool
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define KILO (1024)
#define MEGA (1024*1024)
#define MAX_ITEMS (64*MEGA)
#define swap(v, a, b) {unsigned tmp; tmp=v[a]; v[a]=v[b]; v[b]=tmp;}
#define NUM_THREADS 512
#define MAX_DEPTH 15

struct QuickSortTask {
    int* array;
    unsigned low;
    unsigned high;
    unsigned depth;
    struct QuickSortTask* next;
};

struct ThreadPool {
    pthread_t threads[NUM_THREADS];
    struct QuickSortTask* task_queue_head;
    struct QuickSortTask* task_queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t complete_cond;
    int active_tasks;
    int total_tasks;
    bool shutdown;
};

static int *v;
static struct ThreadPool pool;

static void quick_sort(int *v, unsigned low, unsigned high, unsigned depth);

static void
print_array(void)
{
    int i;
    for (i = 0; i < MAX_ITEMS; i++)
        printf("%d ", v[i]);
    printf("\n");
}

static void
init_array(void)
{
    int i;
    v = (int *) malloc(MAX_ITEMS*sizeof(int));
    for (i = 0; i < MAX_ITEMS; i++)
        v[i] = rand();
}

static void
validate_array(void)
{
    int i;
    for (i = 1; i < MAX_ITEMS; i++) {
        if (v[i-1] > v[i]) {
            printf("Array not sorted at index %d: %d > %d\n", i-1, v[i-1], v[i]);
            return;
        }
    }
    printf("Array is sorted.\n");
}

static unsigned
partition(int *v, unsigned low, unsigned high, unsigned pivot_index)
{
    /* move pivot to the bottom of the vector */
    if (pivot_index != low)
        swap(v, low, pivot_index);

    pivot_index = low;
    low++;

    /* invariant:
     * v[i] for i less than low are less than or equal to pivot
     * v[i] for i greater than high are greater than pivot
     */

    /* move elements into place */
    while (low <= high) {
        if (v[low] <= v[pivot_index])
            low++;
        else if (v[high] > v[pivot_index])
            high--;
        else
            swap(v, low, high);
    }

    /* put pivot back between two groups */
    if (high != pivot_index)
        swap(v, pivot_index, high);
    return high;
}

static void
enqueue_task(struct QuickSortTask* task)
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

static struct QuickSortTask*
dequeue_task(void)
{
    struct QuickSortTask* task = NULL;

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

static void
task_completed(void)
{
    pthread_mutex_lock(&pool.queue_mutex);
    pool.active_tasks--;
    pool.total_tasks--;

    // Signal if all work is done
    if (pool.active_tasks == 0 && pool.total_tasks == 0) {
        pthread_cond_signal(&pool.complete_cond);
    }

    pthread_mutex_unlock(&pool.queue_mutex);
}

static void*
worker_thread(void* arg)
{
    (void)arg;

    while (1) {
        struct QuickSortTask* task = dequeue_task();

        if (task == NULL) {
            // Shutdown signal received
            break;
        }

        quick_sort(task->array, task->low, task->high, task->depth);
        free(task);

        task_completed();
    }

    return NULL;
}

static void
init_thread_pool(void)
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

static void
shutdown_thread_pool(void)
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

static void
wait_for_completion(void)
{
    pthread_mutex_lock(&pool.queue_mutex);

    while (pool.active_tasks > 0 || pool.total_tasks > 0) {
        pthread_cond_wait(&pool.complete_cond, &pool.queue_mutex);
    }

    pthread_mutex_unlock(&pool.queue_mutex);
}

static void
quick_sort(int *v, unsigned low, unsigned high, unsigned depth)
{
    unsigned pivot_index;

    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return;

    /* select the pivot value */
    pivot_index = (low+high)/2;

    /* partition the vector */
    pivot_index = partition(v, low, high, pivot_index);

    /* sort the two sub arrays */
    if (low < pivot_index) {
        if (depth < MAX_DEPTH) {
            // Submit left partition to thread pool
            struct QuickSortTask* task = malloc(sizeof(struct QuickSortTask));
            task->array = v;
            task->low = low;
            task->high = pivot_index - 1;
            task->depth = depth + 1;
            enqueue_task(task);
        } else {
            // Sequential sort for deep recursion
            quick_sort(v, low, pivot_index-1, depth + 1);
        }
    }

    if (pivot_index < high)
        quick_sort(v, pivot_index+1, high, depth + 1);
}

int
main(int argc, char **argv)
{
    struct timespec start, end;
    double elapsed;

    init_array();
    init_thread_pool();

    //print_array();

    // Start timing
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Submit initial task
    struct QuickSortTask* initial_task = malloc(sizeof(struct QuickSortTask));
    initial_task->array = v;
    initial_task->low = 0;
    initial_task->high = MAX_ITEMS - 1;
    initial_task->depth = 0;
    enqueue_task(initial_task);

    // Wait for all tasks to complete
    wait_for_completion();

    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in seconds
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Quick sort time: %.6f seconds\n", elapsed);

    //print_array();

    shutdown_thread_pool();
    validate_array();
    free(v);

    return 0;
}
