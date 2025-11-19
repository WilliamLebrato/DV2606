#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KILO (1024)
#define MEGA (KILO * KILO)
#define MAX_ITEMS (64 * MEGA)
#define MAX_THREADS (64)
#define MIN_SIZE (4096 * 4 * 4 * 4)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[MAX_THREADS];
static int *threads_idle;
static int *v;

typedef struct t_args {
  int low;
  int high;
} t_args;

void swap(int a, int b) {
  int tmp;
  tmp = v[a];
  v[a] = v[b];
  v[b] = tmp;
};

void print_array(void) {
  int i;
  for (i = 0; i < MAX_ITEMS; i++)
    printf("%d ", v[i]);
  printf("\n");
}

int partition(int low, int high, int pivot_index) {
  /* move pivot to the bottom of the vector */
  if (pivot_index != low) {
    swap(low, pivot_index);
  }

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
      swap(low, high);
  }

  /* put pivot back between two groups */
  if (high != pivot_index)
    swap(pivot_index, high);
  return high;
}

int set_busy_thread(int thread_id) {
  pthread_mutex_lock(&mutex);
  threads_idle[thread_id] = 0;
  pthread_mutex_unlock(&mutex);
  return 0;
}

int get_idle_thread(void) {
  for (int i = 0; i < MAX_THREADS; i++) {
    if (threads_idle[i] == 1) {
      set_busy_thread(i);
      return i;
    }
  }
  return -1;
}

int set_idle_thread(int thread_id) {
  pthread_mutex_lock(&mutex);
  threads_idle[thread_id] = 1;
  pthread_mutex_unlock(&mutex);
  return 0;
}

static void init_thread_idle(void) {
  threads_idle = (int *)malloc(MAX_THREADS * sizeof(int));
  for (int i = 0; i < MAX_THREADS; i++)
    threads_idle[i] = 1;
}

void print_thread_idle_and_busy(void) {
  printf("(idle, T_id)\n");
  for (int i = 0; i < MAX_THREADS; i++)
    printf("(%d %d)", threads_idle[i], i);
  printf("\n");
  fflush(stdout);
}

void *quick_sort_multi(void *arg) {
  t_args *parent = (t_args *)arg;
  int pivot_index;
  int low = parent->low;
  int high = parent->high;
  /* no need to sort a vector of zero or one element */
  if (low >= high) {
    return NULL;
  }
  int *local_thread_states = malloc(sizeof(int) * 2);
  local_thread_states[0] = -1;
  local_thread_states[1] = -1;
  t_args *local_thread_args = malloc(sizeof(t_args) * 2);
  pivot_index = (low + high) / 2;
  pivot_index = partition(low, high, pivot_index);
  int idle_thread;

  if (pivot_index < high) {
    if (high - pivot_index >= MIN_SIZE &&
        (idle_thread = get_idle_thread()) != -1) {
      local_thread_args[0].low = pivot_index + 1;
      local_thread_args[0].high = high;
      local_thread_states[0] = idle_thread;
      pthread_create(&threads[idle_thread], NULL, quick_sort_multi,
                     (void *)&local_thread_args[0]);
    } else {
      struct t_args parent_child1 = {pivot_index + 1, high};
      quick_sort_multi((void *)&parent_child1);
    }
  }

  if (low < pivot_index) {
    struct t_args parent_child2 = {low, pivot_index - 1};
    quick_sort_multi((void *)&parent_child2);
  }

  for (int i = 0; i < 2; i++) {
    if (local_thread_states[i] != -1) {
      pthread_join(threads[local_thread_states[i]], NULL);
      set_idle_thread(local_thread_states[i]);
    }
  }
  // Free allocated memory
  free(local_thread_args);
  free(local_thread_states);
  return NULL;
}

static void init_array(void) {
  int i;
  v = (int *)malloc(MAX_ITEMS * sizeof(int));
  for (i = 0; i < MAX_ITEMS; i++)
    v[i] = rand();
}

int confirm_sorted() {
  int i;
  for (i = 0; i < MAX_ITEMS - 1; i++)
    if (v[i] > v[i + 1])
      return 0;
  return 1;
}

void set_all_threads_idle(void) {
  for (int i = 0; i < MAX_THREADS; i++) {
    threads_idle[i] = 1;
  }
}

int main(int argc, char **argv) {
  pthread_mutex_init(&mutex, NULL);
  init_thread_idle();
  init_array();
  t_args *arg_0 = malloc(sizeof(t_args));
  arg_0->low = 0;
  arg_0->high = MAX_ITEMS - 1;
  quick_sort_multi((void *)arg_0);
  free(arg_0);
  pthread_mutex_destroy(&mutex);
  if (confirm_sorted())
    printf("SORTED\n");
  else
    printf("NOT SORTED\n");
  free(v);
}
