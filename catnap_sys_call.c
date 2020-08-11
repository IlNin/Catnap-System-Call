#include <linux/kernel.h> // Needed for KERN_INFO.
#include <linux/module.h> // Needed by all modules.
#include <asm/mwait.h> // Needed for __monitor and __mwait.
#include <linux/syscalls.h> // Needed in order to define a new sys call.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca");

#define TARGET_C1_STATE 0
#define TARGET_C2_STATE 0x10
#define TARGET_C3_STATE 0x20
#define TARGET_C4_STATE 0x30

#define DISABLE_INTERRUPTS_AS_BREAKS 0
#define ALLOW_INTERRUPTS_AS_BREAKS 0x1

#define ENABLE_DEBUG_MODE 0
#define MAX_LOOPS 100

char *hintMSG[4] = {"C1", "C2", "C3", "C4"}; // Strings that are printed during the printk.
char *wakeupMSG[2] = {"Disabled", "Enabled"}; // Same as above.

unsigned long sys_call_table_address = 0xffffffff85e002a0;  // The addess of the system call table. THIS ADDRESS CHANGES AT EACH BOOT OF THE OS: USE 'sudo cat /proc/kallsyms | grep sys_call_table'
unsigned long sys_ni_syscall_address = 0xffffffff84ec1e60; // The address of sys_ni_syscall (x64). THIS ADDRESS CHANGES AT EACH BOOT OF THE OS: USE 'sudo cat /proc/kallsyms | grep sys_ni_syscall'
int sys_ni_syscall_index = 0; // Index of sys_ni_syscall inside the table.

// The system call we'll replace sys_ni_syscall with. 
__SYSCALL_DEFINEx(4, _catnap_backoff, unsigned long*, lock, unsigned long, hint, unsigned long, wakeup_mode, int, thread_id) {
    // Initialize parameters for MWAIT.
    unsigned long HINT;
    unsigned long WAKEUP_MODE;
    int loop_number = 1; // It counts how many time a thread enters in the MWAIT cycle.

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
    
    if (ENABLE_DEBUG_MODE) {
        printk(KERN_ALERT "Thread id: %d    Lock value: %lu\n", thread_id, *lock);
        printk(KERN_ALERT "Thread id: %d    Entering catnap loop!\n", thread_id);
        printk(KERN_ALERT "Thread id: %d    C-State Targetted: %s   Interrupts as breaks: %s\n", thread_id, hintMSG[hint], wakeupMSG[wakeup_mode]);
    }
	
    while (1) {
        __monitor(lock, 0, 0); // mwait knows when there's a write to that address thanks to monitor.
		
        if (*lock) { // Checks if the lock is enabled: if so, it exits from the loop.
            if (ENABLE_DEBUG_MODE)
                printk(KERN_ALERT "Thread id: %d    The lock is already 1!\n", thread_id);
            break; 
        }
		
        else { // Else it goes in the mwait phase until it is woken up.
            if (ENABLE_DEBUG_MODE) 
                printk(KERN_ALERT "Thread id: %d    Entering in mwait state!\n", thread_id);
            __mwait(HINT, WAKEUP_MODE); 
            printk(KERN_ALERT); // For some reason *lock will never gets its current value without printk-ing something.
            if (ENABLE_DEBUG_MODE) 
                printk(KERN_ALERT "Thread id: %d    Exiting mwait state!\n", thread_id);
        }

        if (*lock) { // We check if the node has woken up for the right reasons, meaning if something has changed in the lock. If not, the loop is restarted again.
            if (ENABLE_DEBUG_MODE) 
                printk(KERN_ALERT "Thread id: %d    The lock has now changed to %lu!", thread_id, *lock);
            break;
        }
        
        // DEBUG
        else if (ENABLE_DEBUG_MODE) { // We count how many times a node has woken up for reasons unrelated to a change in the lock.
            printk(KERN_ALERT "Thread id: %d    Looks like I woke up needlessly for the %d time!", thread_id, loop_number);
            loop_number += 1;
        }
        
        if (ENABLE_DEBUG_MODE && loop_number >= MAX_LOOPS) { // If the count is too high, the wait is stopped in order to avoid long (or even infinite) loops.
            printk(KERN_ALERT "Thread id: %d    I have to break the loop or else I'll cycle in eternity since the lock is still %lu!", thread_id, *lock);
            break;
        }
    }
    
    if (ENABLE_DEBUG_MODE) 
        printk(KERN_ALERT "Thread id: %d    Exiting catnap loop!\n", thread_id);
    return 0;
}

// Pointer (and handler) of the system call we have just defined
unsigned long sys_catnap_backoff_ptr  = (unsigned long) __x64_sys_catnap_backoff;

// This function manually sets the 16th bit of cr0 (write control) to 0 or 1, so that we enable/disable writes on the sys_call_table.
inline void mywrite_cr0(unsigned long cr0) {
    asm volatile("mov %0,%%cr0" : "+r"(cr0), "+m"(__force_order));
}

// Enables writes on the sys_call_table.
void enable_write_protection(void) {
    unsigned long cr0 = read_cr0();
    set_bit(16, &cr0); // We want to change the 16th bit of cr0 to 1.
    mywrite_cr0(cr0);
}

// Disables writes on the sys_call_table.
void disable_write_protection(void) {
    unsigned long cr0 = read_cr0();
    clear_bit(16, &cr0); // We want to change the 16th bit of cr0 to 0.
    mywrite_cr0(cr0);
}

// Replaces the system call sys_ni_syscall with catnap.
static int __init overwrite_sys_ni_syscall(void) {
    unsigned long * sys_call_table = (unsigned long *) sys_call_table_address;
    int i;
	
    if (ENABLE_DEBUG_MODE) {
        printk(KERN_ALERT "sys_call_table address: %lx\n", sys_call_table[0]);
        printk(KERN_ALERT "sys_ni_syscall address: %lx\n", sys_ni_syscall_address);
        printk(KERN_ALERT "I'm about to overwrite sys_ni_syscall! Wish me luck!\n");
    }
	
    for (i = 0; i < 256; i++) { // Cycles through the system calls inside the table until it finds the one we want to overwrite.
        if (sys_call_table[i] == sys_ni_syscall_address) {
            if (ENABLE_DEBUG_MODE)
                printk(KERN_ALERT "I've found sys_ni_syscall at index: %d\n", i);
            sys_ni_syscall_index = i;
            break;
        }
    }
	
    disable_write_protection(); // Disable write protection so that we can change the systam call table.
    sys_call_table[sys_ni_syscall_index] = (unsigned long) sys_catnap_backoff_ptr; // Now the table points to our sys call.
    enable_write_protection();// Enable write protection again.
	
    if (ENABLE_DEBUG_MODE) {
        printk(KERN_ALERT "I've overwritten sys_ni_syscall with my system call!\n!");
        printk(KERN_ALERT "init_module completed!\n!");
    }
    return 0;
}

// Restores the original sys_ni_syscall.
static void __exit restore_sys_ni_syscall(void) {
    unsigned long * sys_call_table = (unsigned long *) sys_call_table_address;
	
    if (ENABLE_DEBUG_MODE)
        printk(KERN_ALERT "I'm about to restore sys_ni_syscall! Wish me luck!\n");
	
    disable_write_protection(); // Disable write protection so that we can change the systam call table.
    sys_call_table[sys_ni_syscall_index] = sys_ni_syscall_address; // The original sys call is restored.
    disable_write_protection(); // Enable write protection again.
	
    if (ENABLE_DEBUG_MODE)
        printk(KERN_ALERT "My system call has been discharged from the kernel!\n");
}

module_init(overwrite_sys_ni_syscall); // We indicate which function of this module is supposed to be the 'init' one.
module_exit(restore_sys_ni_syscall); // We indicate which function of this module is supposed to be the 'exit' one.

/*  Original catnap pseudocode used as basis:
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