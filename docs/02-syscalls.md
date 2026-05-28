# syscalls

### Generic memory region management

 * ```sys_mmap```: flag particolari - ```MAP_POPULATE | MAP_NONBLOCK``` (*prefaulting*, note sotto) 
 * ```sys_mprotect```: pkeys ?
 * ```sys_munmap```
 * ```sys_mremap```: supporto sconosciuto
 * ```sys_madvise```: supporto sconosciuto, comunque c'è il *prefaulting*
 * ```sys_brk```
 * ```sys_sbrk```

### Memory locking (no swap-out)

*Importanti??*

 * ```sys_mlock```
 * ```sys_mlock2```
 * ```sys_munlock```
 * ```sys_mlockall```
 * ```sys_munlockall```

### SysV shm IPC

Niente di troppo strano. 

L'shm POSIX è basato sulla creazione di pseudo file in *"/dev/shm/named-shared-memory"*, poi ```mmap```

 * ```sys_shmget```
 * ```sys_shmat```
 * ```sys_shmdt```
 * ```sys_shmctl```

### Processes

 * ```sys_clone```: utilizzato per implementare fork, vfork, LWP (userspace "threads")
 * ```sys_fork```: pesante utilizzo del CoW
 * ```sys_vfork```: condivisione dell'addr. space (stessa page table)
 * ```sys_execve```: sostituzione dell'immagine del processo
 * ```sys_exit```: terminazione completa del processo chiamante
 * ```sys_exit_group```
 * ```sys_kill```
 * ```sys_tkill```
 * ```sys_tgkill```
 * ```sys_ptrace```

### Remote process memory

 * ```sys_process_vm_writev```
 * ```sys_process_vm_readv```

### Other

 * ```sys_readahead```

#### NOTA prefaulting

Con il flag ```MAP_POPULATE``` stiamo dicendo a ```mmap``` di popolare in anticipo le PTE, cioè di non
attendere il primo page fault.

Credo che il supporto sia già presente dato che abbiamo installato hooks in ```handle_pte_fault```:

##### Da ```mmap(MAP_POPULATE)```

 * ```mmap``` (https://elixir.bootlin.com/linux/v7.0.10/source/arch/x86/kernel/sys_x86_64.c#L82)
 * ```ksys_mmap_pgoff``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/mmap.c#L567)
 * ```vm_mmap_pgoff``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/util.c#L565)
   * ```do_mmap``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/mmap.c#L335)
   * ```*populate = len;``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/mmap.c#L563)
 * ```mm_populate``` (https://elixir.bootlin.com/linux/v7.0.10/source/include/linux/mm.h#L3891)
 * ```__mm_populate``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1925)
 * ```populate_vma_page_range``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1813)
 * ```__get_user_pages``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1354)
 * ```faultin_page``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1087)
 * **```handle_mm_fault```** (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1087)

##### Da ```madvise(MADV_POPULATE_READ | MADV_POPULATE_WRITE)```

 * ```madvise``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/madvise.c#L2035)
 * ```do_madvise``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/madvise.c#L2012)
 * ```madvise_do_behaviour``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/madvise.c#L1915)
 * ```madvise_populate``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/madvise.c#L963)
 * ```faultin_page_range``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1887)
 * ```__get_user_pages_locked``` (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1649)
 * **```__get_user_pages```** (https://elixir.bootlin.com/linux/v7.0.10/source/mm/gup.c#L1354)

