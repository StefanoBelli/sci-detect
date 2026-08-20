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

## Kretprobe maxactive

 * Parameter needs proper tuning on certain heavy workload...

 * But don't stress that too much to avoid fast memory exhaustion due to return address space prealloc

 This is actually a more "general concept" rather than snapshot-part-focused

## Improving performance

 * Overhead and memory footprint (as a secondary, minor objective) may be reduced

   * By avoiding the kernel-level stored last page snapshot for each monitored wx-page (avoid multiple memcpys and first buddy allocation)

   * By doing the snapshot only when user actually executes code, for example: wr -> wr -> ex (snap) -> ex -> ex -> wr -> wr -> ex (snap) -> wr -> ex (snap) -> ex

## ```get_user_pages``` before snapshotting

 Do we really need it to do the ```memcpy``` of the page when snapshotting? Let's look at the page fault handler CoW (```wp_page_copy```): they don't use GUP. Are we at risk? Will the page go away under our nose?

 We basically just do ```kmap + memcpy```. It should be enough in our execution context. 

## About ```handle_mm_fault```

 Arch-independent code that handles memory-related faults. 
 
 Reachable mainly through the **arch-dependent page fault handling code** and **GUP**. 

 Note that GUP called when:
  * asking the kernel to prefault pages in (through ```madvise``` or ```mmap``` syscall flags)

  * ```mlock``` the addr space.

  * ```process_vm_{read,write}v```

## Dealing with locking

 Meet per-VMA locks: instead of locking the whole mmap, lock one memory region descriptor.

 Kprobes are placed in various points of the kernel. We need to do stuff on mmaps so we need to acquire the infamous ```mmap_read_lock```.

 Depending on where the probe is placed and on some conditions, it is possible that either the ```mmap_read_lock``` or the interesting per-VMA lock is already taken by prior code.

 We must properly check conditions and context to avoid deadlock conditions. Its a complex situation better described through comments in code.

### Hooks: hpf (```handle_pte_fault```)

 * The most sensible hook if we're talking about locking.

 * Flags passed both as parameters when calling and when returning determine which lock is acquired and if still acquired when returning. Based on these infos we decide lock acquisition.

#### Note: GUP code never acquires the per-VMA lock (only the ```mmap_lock```)

### Hooks: cpr (```change_pte_range```, ```change_protection_range```)

 * We don't acquire the current ```mmap_lock``` since when those function are called, ```mprotect``` code already taken the ```mmap_write_lock```

### Hooks: fsf (```force_sig_info```)

 * We may take the ```mmap_read_lock``` of current without any issues (released earlier, see comments)

## Deadlock issues, lock acquisition ordering

 Subtle situations in which some locks are taken: mmap_locks/per-VMA locks, (split) page table locks, pap lock. 

 We must follow some rules to avoid potential deadlocking.

  * The ```mmap_lock``` is the most sensible one and needs to be acquired before everything else

  * Multiple ```mmap_lock```s need to be held at once, to ensure correct lock acquisition ordering, sort ```mm_struct```s by addr and acquire lock in that exact way.

  * Don't acquire page table locks before acquiring the ptealtprot (pap) lock (don't mix pap_lock -> pt_lock **or** pt_lock -> pap_lock)

  * Check if for ```current->mm``` and its VMA we need to acquire ```mmap_read_lock``` or not (see above)

  * In certain cases it is also needed (see cpr hook) to do the trylock on ```mmap_lock```s anyway to avoid potential deadlock condition

## Reverse mapping

 Is being used to go from the ```folio``` to the ```mm_struct```(s) to the hw PTE entries (via ```page_vma_mapped_walk```)

## Intercepting executes: ```FAULT_FLAG_INSTRUCTION```

 * Looking at the passed ```enum fault_flag flags``` (via ```struct vm_fault* vmf```) to ```handle_pte_fault```, we can see the flag ```FAULT_FLAG_INSTRUCTION```

 * One can think that for valid VMA flags (such that ```vma->flags & VM_EXEC``` is not zero) but hardware PTEs with NX bit on (execute disabled), Linux may just forward the situation to
 ```handle_mm_fault```, more specifically to ```handle_pte_fault``` to clear the hardware NX bit... and you would be **wrong!** (there is only one exception: when page is not present, see initial section
 of this file)

 * Even if the VMA is good-looking, we cannot simply disable execution (on hw PTEs) to be able to intercept it via the ```handle_pte_fault``` hook.

 * The arch-dependent page fault handling code will stop early **anyway** if execute bit disabled - why is that? Probably because Linux doesn't care that much about the execute bit - it will never
 ever disable it if the corresponding VMA has ```VM_EXEC```, it is not like the write bit which is heavily used to do other stuff.

 * That is, if hardware reports instruction fetch issues, just raise a signal (SIGSEGV) to kill the application (```force_sig_fault```)

### How do we intercept executes?

 Just intercept the segmentation fault and handle it properly. There is some minor extra cost to that (we'll have to do some further checks, page table walk, ...) but this is the best way.

 The rationale is that if the SEGV matches our expectation, we just impede the kernel to raise the signal that the thread will deliver to the user when coming back to userspace.

 Kprobe allow us to change registers, so we make our kprobe point to a ```ret``` instruction, to avoid the function to continue. Required patched return thunk due to CPU vuln mitigation.

#### Some notes about SIGSEGV interception

  * Of course didn't affect other signals such as SIGILL, or other stuff

  * We also install hook on ```__bad_area``` to pass essential data to ```force_sig_segv```: the CPU hw-pushed error_code and vma flags before dropping the lock

  * ```force_sig_fault``` also decides what to do based on ```si_code``` (e.g. it is ```SEGV_ACCERR``` or not?)

  * The hook needed to "emulate" a classical page fault handler: check for VMA perms to avoid a loop in some situations (playing with mprotect and stuff...)
    
    - Of course, to proceed, ``` VM_EXEC``` has to be enabled
    - The vma flags are read early in the ```__bad_area``` hook, prior to reaching ```force_sig_fault``` and its hook.
    - This is due to the ```mmap_lock``` / ```per-VMA``` lock still held before imminent release (see ```__bad_area``` kernel code)
    - Later on, via some data structures, the read vma flags are inspected to do the ```VM_EXEC``` check.

  * Moral of the story: the later inspected vma flags are not the "current" ones, but the ones before the lock release (see above), but this is ok, similar to deciding to send ```SIGSEGV```

#### Alternatives?

 * Could use notifier blocks + single stepping?

 * Injecting int3 instr?

 * Manipulating other x86 hw PTE bits to trigger the page fault?

## Kpsleepable

 Q: why we're not allowed to sleep on ```kprobe```s?

 A: because of the per-CPU variable ```current_kprobe``` being set to, explicitly enough, the current kprobe running

 So, how do we go to sleep in ```kprobe```? Little hack, let's find out where the ```current_kprobe``` per-cpu var is located and clear it (```NULL```).

 How do we find the pcp variable? Two ways:

  * ```Kallsyms``` (if available and preferred way)

  * Brute force search (fallback and risky - we may do UAF while dereferencing, triggering KFENCE)

## ```walk_page_range``` vs GUP

 I just needed a way to walk the page tables known the virtual address, don't do anything else! 

 The complex GUP subsystem which also does premature faultin of the pages and caused many problems initially! E.g.
 after the initial ```mmap```, no real access by the application, and no request of ```mlock``` or ```MAP_POPULATE```.

## Be careful to load the kernel module before applications

 Depending on the situation, loading/unloading the module while an application which makes heavy use of WX pages is actively running, may break it (stops working, usually gets a ```SIGSEGV```).

 That's why with CI, examples regarding snapshots are not run, and the module itself is built without snapshot/ptealtprot support. 

 It breaks the CI program behind GitHub Actions because the module is loaded in the midst of its execution.

## ```process_vm_{read,write}v``` and ```FAULT_FLAG_REMOTE```

 Again, remote process VM reading/writing uses GUP (that's important), which may lead to ```handle_mm_fault``` invocation, if needed.

 In this case, ```FAULT_FLAG_REMOTE``` is set and, while ```current``` task is running the hook, GUP code may have taken the ```mmap_read_lock``` OF THE TARGET task's mm, not ```current```.

 Depending on the syscall (being either ```process_vm_readv``` or ```process_vm_writev```) GUP code translates the ```FOLL_WRITE``` to ```FAULT_FLAG_WRITE``` and for both of them
 ```FOLL_REMOTE``` to ```FAULT_FLAG_REMOTE```. Prior ```FOLL_*``` flags being enabled by the aforementioned syscalls when invoking GUP.

 Anyway, they're properly handled in hpf hook code (we know if current kernel control path locks ```current->mm->mmap_lock``` or other task's one, and act consequently)

 Obviously, you can't execute "remote" code (another task in the system) from current task, and you mprotect **your** ```mm``` (which will also affect other threads sharing your addr space, but on the
 kcp ```current->mm```'s ```mmap_lock``` is w/r-locked): fsf and cpr hooks always dealing with ```current->mm``` :)

## ```clone()``` syscall and copying page tables

 The ```clone()``` syscall is the core mechanism to spawn new threads and processes.

 Depending on user-passed parameters (e.g. ```CLONE_VM```) kernel may or may not copy the ```mm``` and hardware PTEs.

 Let's focus on hardware PTEs: if needed, kernel copies page tables, but adopts CoW (that is, sets those PTEs as RO) if needed,
 
 However, if the matching VMA is not anonymous, the kernel will **NOT** copy hardware PTE ranges: VMA operations impl is able to reconstruct
 page tables on demand, avoiding wasting time in copying PTEs (which may not even be used by the forked thread in the first place).

 Call trace:

  * ```kernel_clone``` (https://elixir.bootlin.com/linux/v7.2/source/kernel/fork.c#L2694)
  * ```copy_process``` (https://elixir.bootlin.com/linux/v7.2/source/kernel/fork.c#L1994)
  * ```copy_mm``` (https://elixir.bootlin.com/linux/v7.2/source/kernel/fork.c#L1568)
  * ```dup_mm``` (https://elixir.bootlin.com/linux/v7.2/source/kernel/fork.c#L1527)
  * ```dup_mmap``` (https://elixir.bootlin.com/linux/v7.2/source/mm/mmap.c#L1731)
  * ```copy_page_range``` (https://elixir.bootlin.com/linux/v7.2/source/mm/memory.c#L1507)
  * ```vma_needs_copy``` (https://elixir.bootlin.com/linux/v7.2/source/mm/memory.c#L1482)

 **NOTE**: that the entry point ```kernel_clone``` is called from various points in the kernel (mainly syscalls and function that create kernel threads)

 Now, I thought that it was possible that ```copy_page_range``` made some modifications to hw PTE: in particular that it "adjusted" protection bits of hw PTE to be
 coherent with respect to the associated logical VMA. Well, luckily this is not the case (in order to avoid to interfere with other subsystems?).
  * It will disable write if CoW mapping
  * It will not copy page ranges if VMA permits to ("lazy PTE reconstruction").

 **Why even with the "lazy PTE reconstruction", the ptealtprot mechanism still works?** Because the kprobe is put right at the end of ```handle_pte_fault``` and this
 "lazy PTE reconstruction" mechanism is possible **THANKS** to page faults - that is, we can intercept and enforce our protection bits to be able to detect writes/executes
 properly, according to previous state (due to the lazy PTE reconstruction, obviously, hw PTEs are initially invalid/non-present)

 **The question arises because with this "lazy PTE reconstruction" mechanism, the specific non-anon VMA code will set the hw PTE protection bits according to the VMA flags**,
 causing the missing of detection of some actions: what if we are trying to detect executions, but the hw PTE prot bits are set such that exec is allowed? We miss the page
 fault and hence, the execute event.

## ```handle_pte_fault``` no megalock

 Arch-independent page fault handling code is safe against page table entries modification while it is inspecting it. 

 See the CoW routines, that's what I'm talking about. 

 Why? We change "asynchronously" other PTEs' write bit to intercept writes (in particular, we **always** disable the write bit).

 And that write bit is USED A LOT to handle CoW and other related stuff. 

 While we change this bit (recall that anyway we have acquired the page table lock prior to that) it may happen that page fault handling 
 code is inspecting that specific PTE. 

 Not an issue, it handles the case in which the page table entry has changed in the worst case (anyway the same page table lock is taken...)

 Again, execute bit-related-hardware stuff, kernel, especially the ```handle_pte_fault``` routine doesn't care.

## General pte protection alternation algorithm (HIGH LEVEL VIEW)

 **IMPORTANT**: always disable hw PTE write bit... See more down below.

 * On first WX detection: setup ptealtprot data structure.

   - Do the initial snapshot immediately
   - Initial setup according to the fault type (nofault, write, exec)
     - write: wx detection due to write
     - exec: wx detection due to exec
     - nofault: wx detection due to valid PTE protection change (```mprotect```)
   - Set initial state:
     - write: permit all writes; deny all execs
     - exec: permit all execs; deny all writes
     - nofault: deny everything.

   *NOTE*: we NEVER set the write bit on (hw PTEs)! Always off! This is to avoid any kind of issues with CoW... We only set machine state shadow perms.

 * On second and more... WX detection

   - If denied everything:
     - If this is a write:
       - do the snapshot
       - permit all writes; deny all execs
     - If this is an exec:
       - do the snapshot
       - permit all execs; deny all writes
   - If write permitted (shadow perm):
     - If this is a write (PTE required protection bits missing):
       - For affected VM fault PTE: allow write (at the end of ```handle_pte_fault``` this is safe)
     - If this is an exec (PTE required protection bits missing):
       - Do the snapshot
       - permit all execs; deny all writes
   - If exec permitted (shadow perm);
     - If this is an exec (PTE required protection bits missing):
       - Well, this should never happen...
     - If this is a write (PTE required protection bits missing):
       - Do the snapshot
       - permit all writes; deny all execs. On current kcp, for affected VM fault PTE, set write bit on directly to avoid useless additional page fault.
