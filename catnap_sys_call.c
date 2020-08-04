#include <linux/kernel.h> // Needed for KERN_INFO
#include <linux/module.h> // Needed by all modules
#include <asm/mwait.h> // Needed for __monitor and __mwait
#include <linux/moduleparam.h> // Needed in order to pass arguments as a user
#include <linux/unistd.h> // Needed in order to list all system calls
#include <linux/syscalls.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca");

#define TARGET_C1_STATE 0
#define TARGET_C2_STATE 0x00000010
#define TARGET_C3_STATE 0x00000020
#define TARGET_C4_STATE 0x00000030

#define DISABLE_INTERRUPTS_AS_BREAKS 0
#define ALLOW_INTERRUPTS_AS_BREAKS 0x1

char *hintMSG[4] = {"C1", "C2", "C3", "C4"}; // Strings that are printed during the printk
char *wakeupMSG[2] = {"Disabled", "Enabled"}; // Idem

unsigned long sys_call_table_address = 0xffffffffae6002a0;  // The addess of the system call table. THIS ADDRESS CHANGES AT EACH BOOT OF THE OS.
unsigned long sys_ni_syscall_address = 0xffffffffad6c1e60; // The address of sys_ni_syscall (x64). THIS ADDRESS CHANGES AT EACH BOOT OF THE OS.
int sys_ni_syscall_index = 0; // Index of sys_ni_syscall inside the table

// The system call we'll replace sys_ni_syscall with. 
__SYSCALL_DEFINEx(4, _catnap_backoff, int*, lock, unsigned long, hint, unsigned long, wakeup_mode, int, thread_id) {
    int loop_number = 1; // It counts how many time a thread enters in the MWAIT cycle.
    
    // Initialize parameters for MWAIT
    unsigned long HINT;
    unsigned long WAKEUP_MODE;

    if (hint == 1) {    // Note: All states described belowed are supposed to be general: there could be differences across different processors!
        HINT = TARGET_C2_STATE; // C2: Stop Clock. Stops CPU internal and external clocks via hardware.
    }
    else if (hint == 2) {
        HINT = TARGET_C3_STATE; // C3: Sleep. Stops all CPU internal clocks.
    }
    else if (hint == 3) {
        HINT = TARGET_C4_STATE; // C4: Deeper Sleep. Reduces CPU voltage.
    }
    else {
        HINT = TARGET_C1_STATE; // C1:  Halt. Stops CPU main internal clocks via software; bus interface unit and APIC are kept running at full speed.
        hint = 0;
    }
    
    if (wakeup_mode == 1) { // Treat interrupts as break events even if masked.
        WAKEUP_MODE = ALLOW_INTERRUPTS_AS_BREAKS;
    }
    else { 
        WAKEUP_MODE = DISABLE_INTERRUPTS_AS_BREAKS;
        wakeup_mode = 0;
    }
    
    printk(KERN_ALERT "Thread id: %d    Lock value: %d\n", thread_id, *lock);
    printk(KERN_ALERT "Thread id: %d    Entering catnap loop!\n", thread_id);
    printk(KERN_ALERT "Thread id: %d    C-State Targetted: %s   Interrupts as breaks: %s\n", thread_id, hintMSG[hint], wakeupMSG[wakeup_mode]);
	
    while (1) {
        __monitor(lock, 0, 0); // mwait knows when there's a write to that address thanks to monitor
		
        if (*lock) { // Checks if the lock is enabled: if so, it exits from the loop
            printk(KERN_ALERT "Thread id: %d    The lock is already 1!\n", thread_id);
            break; 
        }
		
        else { // Else it goes in the mwait phase until it is woken up
            printk(KERN_ALERT "Thread id: %d    Entering in mwait state!\n", thread_id);
            __mwait(HINT, WAKEUP_MODE); 
            printk(KERN_ALERT "Thread id: %d    Exiting mwait state!\n", thread_id);
        }

        if (*lock) { // We check if the node has woken up for the right reasons: something has changed in the lock. If not, the loop is restarted again.
            printk(KERN_ALERT "Thread id: %d    The lock has now changed to %d!", thread_id, *lock);
            break;
        }
        
        else {
            printk(KERN_ALERT "Thread id: %d    Looks like I woke up needlessly for the %d time!", thread_id, loop_number);
            loop_number += 1;
        }
        
        // DEBUG
        if (loop_number >= 100) {
            printk(KERN_ALERT "Thread id: %d    I have to break the loop or else I'll cycle in eternity", thread_id);
            break;
        }
    }
    
    printk(KERN_ALERT "Thread id: %d    Exiting catnap loop!\n", thread_id);
    return 0;
	
    /*  Original catnap pseudocode:
        1: while True do
            2: monitor_target ←address_of (locked)
            3: M ON IT OR (monitor_target)
            4: if Locked then
                5: break
            6: else
                7: hint ←designated_C–state
                8: wakeup_mode ←interrupt_wakeup_disable
                9: M W AIT (hint, wake_up_mode)
            10: end if
            11: if Locked then
                12: break
            13: end if
            14: end while */
}

// Pointer (and handler) of the system call we have just defined
unsigned long sys_catnap_backoff_ptr  = (unsigned long) __x64_sys_catnap_backoff;

// For some reason 'write_cr0 (read_cr0 () & (~ 0x10000))' didn't work, so I had to  copy this function that manually sets the 16th bit (write control) to 1
inline void mywrite_cr0(unsigned long cr0) {
    asm volatile("mov %0,%%cr0" : "+r"(cr0), "+m"(__force_order));
}

void enable_write_protection(void) {
    unsigned long cr0 = read_cr0();
    set_bit(16, &cr0);
    mywrite_cr0(cr0);
}

void disable_write_protection(void) {
    unsigned long cr0 = read_cr0();
    clear_bit(16, &cr0);
    mywrite_cr0(cr0);
}

// Replace the system call sys_ni_syscall 
static int __init overwrite_sys_ni_syscall(void) {
    unsigned long * sys_call_table = (unsigned long *) sys_call_table_address;
    int i;
	
    printk(KERN_ALERT "sys_call_table address: %lx\n", sys_call_table[0]);
    printk(KERN_ALERT "sys_ni_syscall address: %lx\n", sys_ni_syscall_address);
    printk(KERN_ALERT "I'm about to overwrite sys_ni_syscall! Wish me luck!\n");
	
    for (i = 0; i < 256; i++) { // Cycles through the system calls inside the table until it finds the one we want to overwrite
        // printk(KERN_ALERT "Searching in index %d with address %lx\n", i, sys_call_table[i]);
        if (sys_call_table[i] == sys_ni_syscall_address) {
            printk(KERN_ALERT "I've found sys_ni_syscall at index: %d\n", i);
            sys_ni_syscall_index = i;
            break;
        }
    }
	
    disable_write_protection(); // Disable write protection so that I can change the systam call table
    sys_call_table[sys_ni_syscall_index] = (unsigned long) sys_catnap_backoff_ptr;
    enable_write_protection();// Enable write protection
	
    printk(KERN_ALERT "I've overwritten sys_ni_syscall with my system call!\n!");
    printk(KERN_ALERT "init_module completed!\n!");
    return 0;
}

// Restore the original sys_ni_syscall
static void __exit restore_sys_ni_syscall(void) {
    unsigned long * sys_call_table = (unsigned long *) sys_call_table_address;
	
    printk(KERN_ALERT "I'm about to restore sys_ni_syscall! Wish me luck!\n");
	
    disable_write_protection(); // Disable write protection so that I can change the systam call table
    sys_call_table[sys_ni_syscall_index] = sys_ni_syscall_address;
    disable_write_protection(); // Enable write protection
	
    printk(KERN_ALERT "My system call has been discharged from the kernel!\n");
}

module_init(overwrite_sys_ni_syscall);
module_exit(restore_sys_ni_syscall);