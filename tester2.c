#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

static int locks[2] = {1, 0};

void *delayed_system_call(void * p_index) {
	for (int i = 0; i < 1000000; i++) { 
        continue;
    }
    
    int i = *((int *) p_index);
    printf("I'm process ID: %d and I'm invoking the catnap system call!\n", i);
    syscall(134, &locks[i], 2, 0, i); // sys_ni_syscall has index 134
    locks[i+1] = 1; 
    printf("I'm process ID: %d and it looks like everything worked out ok!\n", i);
}

void *call_system_call(void * p_index) {
    int i = *((int *) p_index);
    printf("I'm process ID: %d and I'm invoking the catnap system call!\n", i);
    syscall(134, &locks[i], 2, 0, i); // sys_ni_syscall has index 134
    printf("I'm process ID: %d and it looks like everything worked out ok!\n", i);
}


int main(int argc, char** argv) {
    // Allocates memory for nThreads threads
    pthread_t *threads = malloc(sizeof(pthread_t)*2);

    // Starts the threads
    for (int i = 0; i < 2; i++) {
        int *p_index = malloc(sizeof(int*));
        *p_index = i;
        if (i == 0) {
            pthread_create(&threads[i], NULL, delayed_system_call, (void*) p_index); 
        }
        else {
            pthread_create(&threads[i], NULL, call_system_call, (void*) p_index);
        }
    }

    // Waits until all threads are finished
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}