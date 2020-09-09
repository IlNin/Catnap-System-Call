# Catnap-System-Call
A Linux kernel module that defines a new system call that implements a back-off policy called 'catnap'.<br/>
Link to the original paper: https://ieeexplore.ieee.org/document/8978918


<b>Description:</b><br/>
The kernel module catnap_sys_call replaces a system call that does absolutely nothing (sys_ni_syscall), with one that implements the catnap back-off algorithm. This waiting policy is supposed to be a middle ground between spinning and sleeping, since threads are technically considered to be active by the system, even though in reality they operating within processors that are working in a low profile/optimized fashion.<br/>
This approach is supposed to save up on performance and energy consumption, and it's achieved by using the MONITOR/MWAIT instructions, which can be executed only at privilege level 0, hence the need for a system call.<br/>
- MONITOR: This instruction monitors an address range that has been given in input. If someone writes in that address range, MONITOR is notified and wakes up any   thread that is waiting with MWAIT, an instruction which MONITOR is paired with. MONITOR also accepts two other inputs, an hint and an extension, but they are both generally initialized with 0 since they are not supported by most processors.
- MWAIT: If paired with MONITOR, any thread that encounters this instruction will stop until the address range observed by MONITOR is written by someone or until an external factor breaks the wait, such as debug/machine check interruptions and signals like INIT and RESET. MWAIT accepts two inputs: a hint, which specifies in which C-state the processor should go in during the wait, and a wakeup_mode, which specifies if interruptions, even the masked ones, should interrupt the wait.


<b>Testing:</b><br/>
Once finished working on the module, I added four new policies that are based on different variants of Catnap to Litl, which is a tool that allows executing a program based on Pthread mutex locks with other locking algorithms. Then I used a program which was kindly provided by my professor, Lockbench, to simulate programs that are automatically executed through Litl in order to gain some stats about the efficacy of Catnap when used by MCS. You can find the results in my report published in this repository, while the policies used in such tests can be obtained in your copy of Litl by using a patch that is also provided here.<br/>
Litl: https://github.com/multicore-locks/litl<br/>
Lockbench:https://github.com/HPDCS/lockbench


<b>Comments:</b><br/>
This was a small and yet time consuming project, because there were a lot of concepts that I had to learn before writing a single line of code. Kernel modules, system call tables, locking algorithms and policies, the C-States, the MONITOR/MWAIT instructions themselves: before working on this project, my knowledge in all these topics was minimal at best and non-existent at worst. Thankfully my professor was always at my side, telling me where to focus and suggesting me various approaches, without explicitally forcing me on a single and particular solution, meaning that I had a lot of freedom, and for that I thank him.<br/>
At the end everything worked out just fine, because in the tests Catnap performed exactly as the paper suggested, both in the good and in the bad.


<b>Install Instructions:</b><br/>
1 - Check if your machine supports MONITOR/MWAIT by using the command 'cpuid' on your command line. If you don't have it installed, use 'sudo apt-get install cpuid'.<br/>
2 - Make sure that the addresses for the sys_call_table and sys_ni_syscall are correct in catnap_sys_call.c, since they change every time you boot the system. Type 'sudo cat /proc/kallsyms | grep sys_ni_syscall' and 'sudo cat /proc/kallsyms | grep sys_call_table' to get their current value.<br/>
3 - Make the Makefile.<br/>
4 - Insert the module into the kernel by typing 'sudo insmod ./catnap_sys_call.ko'. You can also remove it by typing 'sudo rmmod catnap_sys_call'.<br/>
5 - Compile and execute the tester and see if the module works correctly! In order to see the results after each test, make sure you have enabled the 'Debug Mode' in the module.<br/>
