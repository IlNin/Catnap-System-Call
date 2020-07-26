#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

static int nThreads = 12;
static int locks[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

void *call_system_call(void * p_index) {
	int i = *((int *) p_index);
	printf("I'm process ID: %d and I'm invoking the catnap system call!\n", i);
	syscall(134, locks[i]); // sys_ni_syscall has index 134
	locks[i+1] = 1; 
	printf("I'm process ID: %d and it looks like everything worked out ok!\n", i);
}

int main(int argc, char** argv) {

	// Allocates memory for nThreads threads
    pthread_t *threads = malloc(sizeof(pthread_t)*nThreads);
	int *p_index = malloc(sizeof(int*));
	*p_index = 0;
	pthread_create(&threads[0], NULL, call_system_call, (void*) p_index);
	pthread_join(threads[0], NULL);

    /* Starts the threads and takes the begin time
    for (int i = 0; i < nThreads; i++) {
		int *p_index = malloc(sizeof(int*));
		*p_index = i;
        pthread_create(&threads[i], NULL, call_system_call, (void*) p_index);
    }

    // Waits until all threads are finished
    for (int i = 0; i < nThreads; i++) {
        pthread_join(threads[i], NULL);
    } */
    return 0;
}