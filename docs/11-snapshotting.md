# Snapshot notes

## About the page fault handler

 * ```handle_pte_fault``` being called on **non-present page** on **instruction fetch**. Doesn't matter whether ```VM_EXEC``` is enabled or not.

   This time, the pushed error_code by the CPU is 0x14

    * if ```VM_EXEC``` is enabled then ```handle_pte_fault``` will populate the PTE properly (NX bit disabled)
    * if ```VM_EXEC``` is not enabled then ```handle_pte_fault``` will enable the NX bit (denying execution)

    So, when returning from the first exception, the CPU **reexecutes the same faulting instruction**:

     * If the setupped PTE allows exec: the user thread goes without any exception
     * Otherwise, **for the same instruction** a second exception is raised by the CPU. This time the page **is present** while attempting **instruction fetch**.
     On this second exception, the Linux kernel page fault handler never reaches arch-independent code (```handle_mm_fault```) and delivers a forced signal
     ```SIGSEGV``` with ```SEGV_ACCERR```, error 0x15 (the error_code pushed by the CPU, shown in dmesg thanks to ```show_signal_msg```)

      * ```exc_page_fault``` (https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L1483)
      * ```handle_page_fault``` (https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L1462)
      * ```do_user_addr_fault``` (https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L1207)
      * ```access_error``` (being called here: https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L1329)
        * ```error_code & X86_PF_PROT``` (fails here: https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L1105)
      * ```bad_area_access_error```(https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L868)
      * ```__bad_area``` (https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L834)
      * ```__bad_area_nosemaphore``` (mmap/per-vma locks released, https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L834)
        * ```show_signal_msg``` (shows segfault in dmesg, https://elixir.bootlin.com/linux/v7.1.3/source/arch/x86/mm/fault.c#L744)
      * ```force_sig_fault``` (https://elixir.bootlin.com/linux/v7.1.3/source/kernel/signal.c#L1701)

    This explains why:

    ```c

    char *mem = mmap(PROT_READ | PROT_WRITE);
    ((void(*)(void))mem)();

    ```

    Results in segmentation fault with error code 0x15, that is, we expected the page to not be present at the time of instr fetch, while it is.

 * However, when the first non-present, denied access (that is, the associated vma doesn't have ```VM_WRITE```) to some memory is caused by a write, 
 the kernel delivers the ```SIGSEGV``` immediately, there are **not** 2 page faults. Based on the ```error_code```, ```access_error``` immediately detects that the write
 is to a memory with an associated VMA without ```VM_WRITE```. 

    The situation is the following:

    ```c
 
    char *mem = mmap(PROT_READ);
    *mem = 'a';

    ```

    In fact, ```show_signal_msg``` shows an error of 6 (0b110), which means: user + write + non-present.

    This demonstrates that there was no ```handle_pte_fault``` execution to populate PTEs (make them present), execution of the thread running in kernel mode was blocked
    earlier.
