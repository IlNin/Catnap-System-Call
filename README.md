# Catnap-System-Call
A Linux kernel module that defines a new system call that implements a back-off policy called 'catnap'.


<b>Description:</b><br/>
The kernel module catnap_sys_call replaces a system call that does absolutely nothing (sys_ni_syscall), with one that implements the catnap back-off algorithm. This waiting policy is supposed to be a middle ground between spinning and sleeping, since threads are technically considered to be active by the system, even though in reality they operating within processors that are working in a low profile/optimized fashion.<br/>
This approach is supposed to save up on performance and energy consumption, and it's achieved by using the MONITOR/MWAIT instructions, which can be executed only at privilge level 0, hence the need for a system call.<br/>
- MONITOR: This instruction monitors an address range that has been given in input. If someone writes in that address range, MONITOR is notified and wakes up any   thread that is waiting with MWAIT, an instruction which MONITOR is paired with. MONITOR also accepts two other inputs, an hint and an extension, but they are both generally initialized with 0.
- MWAIT: If paired with MONITOR, any thread that encounters this instruction will stop until the address range observed by MONITOR is written by someone or until an external factor shows itself, such as debug/machine check interruptions and signal like INIT and RESET. MWAIT accepts two inputs: a hint, which specifies in which C-state the processor should go during the wait, and a wakeup_mode, which specifies if interruptions, even the masked ones, should interrupt the wait.


<b>Comments:</b><br/>
Work-in-progress.


<b>Install Instructions:</b><br/>
1 - Make sure that the addresses for the sys_call_table and sys_ni_syscall are correct in catnap_sys_call.c, since they change every time you boot the system. Type 'sudo cat /proc/kallsyms | grep sys_ni_syscall' and 'sudo cat /proc/kallsyms | grep sys_call_table' to get their current value.<br/>
2 - Make the Makefile.<br/>
3 - Insert the module into the kernel by typing 'sudo insmod ./catnap_sys_call.ko'. You can also remove it by typing 'sudo rmmod catnap_sys_call'.<br/>
4 - Compile and execute the tester and see if the module works correctly!<br/>


Link to the original paper: https://ieeexplore.ieee.org/document/8978918
