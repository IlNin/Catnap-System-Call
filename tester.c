#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define nThreadsMax 6

int input; // The type of test selected by the user.
int hint; // Attribute that indicates the C-State targetted. Used by mwait.
int wakeup_mode; // Attribute that indicates if interruptions are able to break mwait, even if masked. Used by mwait.

static int locks[nThreadsMax]; // Each thread has its own lock.

void *call_system_call(void * p_index) {
    int i = *((int *) p_index);
    printf("Thread id: %d   Invoking the catnap system call!\n", i);
    syscall(134, &locks[i], hint-1, wakeup_mode-1, i); // sys_ni_syscall, which has been replaced by catnap, has index 134.
    locks[i+1] = 1; 
    printf("Thread id: %d   Returning from the call!\n", i);
}

void *delayed_system_call(void * p_index) {
	for (int i = 0; i < 1000000; i++) { 
        continue;
    }
    
    int i = *((int *) p_index);
    printf("Thread id: %d   Invoking the catnap system call!\n", i);
    syscall(134, &locks[i], hint-1, wakeup_mode-1, i); // sys_ni_syscall, which has been replaced by catnap, has index 134.
    locks[i+1] = 1; 
    printf("Thread id: %d  Returning from the call!\n", i);
}

void test1(void) {
    // Allocates memory for nThreads threads
    pthread_t *threads = malloc(sizeof(pthread_t)*2);
    
    // Populates the lock array
    locks[0] = 1;
    for (int i = 1; i < nThreadsMax; i++) {
        locks[i] = 0; 
    }

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
}

void test2(void) {
    // Allocates memory for nThreads threads
    pthread_t *threads = malloc(sizeof(pthread_t)*nThreadsMax);
	
    // Populates the lock array
    locks[0] = 1;
    for (int i = 1; i < nThreadsMax; i++) {
        locks[i] = 0; 
    }

    // Starts the threads
    for (int i = 0; i < nThreadsMax; i++) {
        int *p_index = malloc(sizeof(int*));
        *p_index = i;
        pthread_create(&threads[i], NULL, call_system_call, (void*) p_index);
    }

    // Waits until all threads are finished
    for (int i = 0; i < nThreadsMax; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char** argv) {
    printf(" ____     __     _____   __     _     __      ____    \n");
    printf("|        |  |      |     | \\    |    |  |    |    |  \n");
    printf("|       | __ |     |     |  \\   |   | __ |   |___/   \n");
    printf("|      |      |    |     |   \\  |  |      |  |       \n");
    printf("|____  |      |    |     |    \\_|  |      |  |       \n");
    printf("                                   _____________      \n");
    printf("                                  | T E S T E R |     \n");
    printf("                                   \\___________/     \n");
    printf("\n");

    printf("This program is meant to test if the catnap system call is working correctly and its efficacy!\n");
    printf("Choose between these two tests: \n");
    printf("1 - Two threads make the call: one immediately with lock 0 and the other with lock 1, but only after a delay.\n");
    printf("    The thread that makes the first call is supposed to stay still in mwait mode until the second thread writes 1 in its lock.\n");
    printf("    This test is useful to see how many times thread 1 wakes up while waiting for thread 2!\n");
    printf("2 - Six threads make the call: the first with lock 1, the others with lock 0.\n");
    printf("    The threads are supposed to finish their execution in a precise order, since thread n writes 1 in the lock of thread n+1 (which is\n");
    printf("    waiting in mwait) only when it has returned from the system call.\n");
    printf("    This test is useful to see how many times a thread wakes up while waiting for its previous one in line to finish its execution.\n");
    printf("\n");
    printf("Type the number of the test you want to execute. Feeling lost? Press '3' then!\n");

    while (1) {
        scanf("%d", &input);
        if (input == 1) {
            printf("You have choosen the first test!\n"); 
            break;
        }
        else if (input == 2) {
            printf("You have choosen the second test!\n"); 
            break;
        }
        else if (input == 3) {
            break;
        }
        else {
            printf("Invalid input!\n"); 
        }
    }
    printf("\n");
    
    if (input == 1 || input == 2) { 
        printf("Which C-State do you want to target when going in mwait?\n"); 
        printf("1 - C1: Halt. Stops CPU main internal clocks via software; bus interface unit and APIC are kept running at full speed. (Suggested)\n");
        printf("2 - C2: Stop Clock. Stops CPU internal and external clocks via hardware.\n");
        printf("3 - C3: Sleep. Stops all CPU internal clocks.\n");
        printf("4 - C4: Deeper Sleep. Reduces CPU voltage.\n");
        printf("NOTE: All states described above are supposed to be general: there could be differences across the various processors!\n");
        printf("\n");

        while (1) {
            scanf("%d", &hint);
            if (hint == 1) {
                printf("You have chosen to target C1!\n");
                break;
            }
            else if (hint == 2) {
                printf("You have chosen to target C2!\n");
                break;
            }
            else if (hint == 3) {
                printf("You have chosen to target C3!\n");
                break;
            }
            else if (hint == 4) {
                printf("You have chosen to target C4!\n");
                break;
            }
            else {
                printf("Invalid input!\n"); 
            }
        }
        printf("\n");
        
        printf("You want interruptions to break the mwait state even if masked?\n");
        printf("1 - No. (Suggested)\n");
        printf("2 - Yes.\n");
        printf("\n");
        
        while (1) {
            scanf("%d", &wakeup_mode);
            if (wakeup_mode == 1) {
                printf("You have chosen to ignore interruptions, even the masked ones!\n");
                break;
            }
            else if (wakeup_mode == 2) {
                printf("You have chosen to enable interruptions to break from mwait, even the masked ones!\n");
                break;
            }
            else {
                printf("Invalid input!\n"); 
            }
        }
        printf("\n");
        
        if (input == 1) {
            test1(); 
        }
        else if (input == 2) {
            test2();
        }
        printf("\n");
        
        printf("The test is over! You can check what the catnap system calls did by typing 'sudo dmesg -c' on the terminal.\n");
    }
    
    else {
        printf("The kernel module catnap_sys_call replaces a system call that does absolutely nothing (sys_ni_syscall), with one that implements\n");
        printf("the catnap back-off algorithm. This waiting policy is supposed to be a middle ground between spinning and sleeping, since threads\n");
        printf("are technically considered to be active by the system, even though in reality they operating within processors that are working\n");
        printf("in a low profile fashion. This approach is supposed to save up on performance and energy consumption, and it's achieved by using\n");
        printf("the MONITOR/MWAIT instructions, which can be executed only at privilge level 0, hence the need for a system call.\n");
        printf("- MONITOR: This instruction monitors an address range that has been given in input. If someone writes in that address range,\n");
        printf("           MONITOR is notified and wakes up any thread that is waiting with MWAIT, an instruction whose MONITOR is paired with.\n");
        printf("           MONITOR also accepts two other inputs, an hint and an extension, but they are both generally initialized with 0.\n");
        printf("- MWAIT: If paired with MONITOR, any thread that encounters this instruction will stop until the address range observed by MONITOR\n");
        printf("         is written by someone or until an external factor shows itself, such as debug/machine check interruptions and signals\n");
        printf("         like INIT and RESET. MWAIT accepts two inputs: a hint, which specifies in which C-state the processor should go during\n");
        printf("         the wait, and a wakeup_mode, which specifies if interruptions, even the masked ones, should interrupt the wait.\n\n");  
        printf("INSTRUCTIONS:\n");
        printf("1 - Make sure that the addresses for the sys_call_table and sys_ni_syscall are correct in catnap_sys_call.c, since they change\n");
        printf("    every time you boot the system. \n");
        printf("    Type 'sudo cat /proc/kallsyms | grep sys_ni_syscall' and 'sudo cat /proc/kallsyms | grep sys_call_table' to get their current\n");
        printf("    value.\n");
        printf("2 - Make the Makefile.\n");
        printf("3 - Insert the module into the kernel by typing 'sudo insmod ./catnap_sys_call.ko'. You can also remove it by typing\n");
        printf("    'sudo rmmod catnap_sys_call'.\n");
        printf("4 - Execute this test and see if the module works correctly!\n");
        printf("\n");
    }
        
    return 0;
}
