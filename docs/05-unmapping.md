# Unmapping user pages

### Function calls

 * ```sys_munmap``` https://elixir.bootlin.com/linux/v7.1/source/mm/mmap.c#L1076
 * ```__vm_munmap``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L3275
 * ```do_vmi_munmap``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L1630
 * ```do_vmi_align_munmap``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L1583
 * ```vms_complete_munmap_vmas``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L1330
 * ```vms_clear_ptes``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L1275
 * ```unmap_region``` https://elixir.bootlin.com/linux/v7.1/source/mm/vma.c#L481
    - **```tlb_gather_mmu```** https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L462
    - ```unmap_vmas```  https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L2145
    - ```__zap_vma_range``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L2075
    - ```zap_p4d_range``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L2056
    - ```zap_pud_range``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L2028
    - ```zap_pmd_range``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L1992
    - **```zap_pte_range```** https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L1900
    - ```do_zap_pte_range``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L1806
    - ```zap_present_ptes``` https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L1689
    - **```zap_present_folio_ptes```** https://elixir.bootlin.com/linux/v7.1/source/mm/memory.c#L1638
    - **```tlb_delay_rmap```** https://elixir.bootlin.com/linux/v7.1/source/include/asm-generic/tlb.h#L303
    - **```__tlb_remove_folio_pages```** https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L207
    ...
    - **```tlb_finish_mmu```** https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L508
    - ```tlb_flush_mmu``` https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L421
    - ```tlb_flush_mmu_free``` https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L413
    - **```tlb_batch_pages_flush```** https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L146
    - ```__tlb_batch_free_encoded_pages``` https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L103
    - ```free_pages_and_swap_cache``` https://elixir.bootlin.com/linux/v7.1/source/mm/swap_state.c#L385
    - **```folios_put_refs```** https://elixir.bootlin.com/linux/v7.1/source/mm/swap.c#L957
    - **```free_unref_folios```** https://elixir.bootlin.com/linux/v7.1/source/mm/page_alloc.c#L2987

Most important calls are highlighted.

### ```mmu_gather```

The first one (```tlb_gather_mmu```) initializes a very important on-stack variable named "tlb" of type ```struct mmu_gather```

Def of ```struct mmu_gather```: https://elixir.bootlin.com/linux/v7.1/source/include/asm-generic/tlb.h#L325

Some docs are available here: https://elixir.bootlin.com/linux/v7.1/source/include/asm-generic/tlb.h#L31

If ```CONFIG_MMU_GATHER_NO_GATHER``` is not set (99% of the cases, the remaining 1% is s390 arch) then collect pages to be freed
in **batches** (```struct mmu_gather_batch```), which are later scanned to do the freeing, by ```tlb_batch_pages_flush```.

In those batches, there are collected ```struct encoded_page```s, 
which is documented: https://elixir.bootlin.com/linux/v7.1/source/include/linux/mm_types.h#L224

Basically the lowest bits (2 LSBs) of struct page* ptr is used to store some contextual infos, most notably the NR_PAGES bit,
which is documented: https://elixir.bootlin.com/linux/v7.1/source/include/linux/mm_types.h#L245

In ```struct mmu_gather```, there are 2 members:
 * ```struct mmu_gather_batch *active;```: the currently active batch
 * ```struct mmu_gather_batch local;```

Initially, ``` active = &local ```.

When inspecting, ```tlb_batch_pages_flush``` scans from the local batch to ->next, ->next, ... to do the freeing
by calling ```__tlb_batch_free_encoded_pages```, to which a batch of the linked list is passed, each time the element is scanned
Note that this last one function does ```cond_resched()``` every 512 folios to free to avoid soft lockups. This ultimately calls
```free_pages_and_swap_cache```

```__tlb_remove_folio_pages``` is the one that adds the encoded page to the active batch, which may point to the local 
batch or its ->next or its ->next->next and so on (forming a linked list of batches). For sake of simplicity - this function
collects in the active batch, and when full, creates another batch (link to the linked list) and make this last one, the active one.

There is a limit for the number of ```struct encoded_page```s collected in every ```struct mmu_gather_batch``` and the number of batches
contained in each ```struct mmu_gather```.

### ```free_pages_and_swap_cache```

Basically this converts the ```struct encoded_page```s to a batch of folios ```struct folio_batch```.

It creates a new batch of folios (by calling ```folio_batch_init```), so it scans the ```encoded_page```s.

There is a ```refs``` array of ```FOLIO_BATCH_SIZE``` integers, matching each folio in the batch. This array will be
initialized during the scanning of ```encoded_page```s.

For each ```encoded_page```:
 - Zero the 2 LSBs of the encoded page, so we get back the ptr to ```struct page``` descriptor (thanks to https://elixir.bootlin.com/linux/v7.1/source/include/linux/mm_types.h#L265)
 - Get the folio of the page
 - **Attempt** to free swap space used for the folio
 - Establish number of refs for the folio, by setting the ```refs[folios.nr]``` to either:
  - 1 if the next element of the ```encoded_page```s to be scanned, is **not** a NR_PAGES information
  - nr_pages otherwise (```ENCODED_PAGE_BIT_NR_PAGES_NEXT``` is set for current ```encoded_page```).  This info is taken by looking at the next element
 - Add the folio to the batch. If it gets full with this insertion, do the ```folios_put_refs```

At the end, if ```folios.nr > 0``` do ```folios_put_refs```

Why ```refs[folios.nr]``` ? Because the ```folio_batch``` is being built on the fly, so ```folios.nr``` indicates 0,1,2,... to match refs correctly to the folio index in batch.

### ```folios_put_refs```

Reduce the reference count on a batch of folios. Call ```free_unref_folios``` for those folios (grouped in a ```folio_batch```) that reach 0 after put operation.

It gets a ```struct folio_batch*``` and associated ```unsigned int *refs``` array. 

This array may be NULL, in this case the number of refs to be decremented for the put operation on every folio of the batch is 1. Otherwise is ```refs[i]```.

The loop gets through all ```folios->nr``` folios.

For each folio it does ```folio_ref_sub_and_test(folio, nr_refs)```. 

If this returns false, refcount didn't reach 0: ```continue;```.

If it returns true, refcount for the folio reached 0, it must be freed:
 - Remove it from a LRU list (page cache release)
 - By using the same ```folio_batch``` and another index ```j``` group this in order to be freed.
 - Increment ```j```

If ```j``` is not 0, there are folios to free! Call ```free_unref_folios``` with ```folios->nr = j``` as nr of folios in the batch.

Otherwise, if ```j``` is 0, reinit the passed ```folio_batch``` ("remove all folios from that batch")

## When

See ```tlb_next_batch``` (https://elixir.bootlin.com/linux/v7.1/source/mm/mmu_gather.c#L20)

These operations may be done when ```tlb_finish_mmu``` is called by ```unmap_region``` (most likely) 
or earlier (see ```zap_pte_range``` and similar) if ```force_flush``` is enforced:
 - delayed rmaps removal are pending, and active batch is not local batch.
 - number of batches in the ```mmu_gather``` reached its max (```MAX_GATHER_BATCH_COUNT```)
 - memory pressure (```__get_free_pages(GFP_NOWAIT) returns NULL```)

## Why the folio refcnt may be greater than 0? And cause the ```current``` kernel control path (```sys_munmap```) not to free the physical page? This impacts testing!

In particular, this question arises when we do the unmap of a ```MAP_ANONYMOUS | MAP_PRIVATE``` page (or whatever page that we know we were the only one actively referencing it, before the unmap),
causing the page to not being freed immediately (sync. wrt. ```sys_munmap```).

Suppose classical one process, made of one thread (the distinction between the two is very thin) setup, we do ```mmap```, access the page in write mode and then ```munmap```. As a result:
 * the PTE is "zapped" (process can't access the memory anymore)
 * before the return to user mode the TLB must be flushed
 * **the hw pte-referenced physical page may not be immediately returned to the buddy allocator**

So, why? 

Well, Linux has the concept of ```struct folio_batch``` (before the introduction of folios, they were known as ```struct pagevec```), which is a "container" (linked list, but we don't care) of folios
on which the kernel does operations, see above for one example (```free_pages_and_swap_cache``` collects folios in batch and calls ```folios_put_refs``` on it).

The most important thing those batches do is that they **accumulate** folios before moving them to the LRU lists of the kernel (used by PFRA to determine which folios to evict first). 
Why is that? Because using these lists requires holding a global lock (wrt the multiple LRU lists) which severely impacts performance.

The lock is ```struct lruvec::lru_lock``` which is a spinlock that protects the ```struct lru_lock::lists[NR_LRU_LISTS]```, 
see https://elixir.bootlin.com/linux/v7.1.2/source/include/linux/mmzone.h#L757 .

As an optimization, instead of doing the LRU operation for each folio immediately and requiring **for each folio** to get the ```lru_lock```, we accumulate a certain number of folios in a batch 
and do that operation on those folios once the batch gets full. The advantage here is that the lock is held once for (for example) 15 folios, instead of holding/releasing it for 15 times
(impacts on CPU caches) for every single folio.

Given the background, lets get more into the details: the kernel defines a ```struct cpu_fbatches``` (see https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L50), and a per-CPU variable of
that type (symbol name ```cpu_fbatches```, see https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L68). This means that for each CPU the kernel holds a **set of batches of folios**.

Those multiple batches are required because in there we collect folios for which we will do the same particular operation once the batch gets full. This involves moving the i-th folio contained in
the batch, into a LRU list.

The core function that does that is ```folio_batch_move_lru``` (https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L158): this gets the correct ```lruvec```, holds the ```lru_lock```, and
for each folio in the passed ```folio_batch``` does the ```move_fn``` operation passed as callback, "moving" the folio to the target LRU list, and, in the end, emptying the per-CPU ```folio_batch```.

 * Calls ```folio_lruvec_relock_irqsave``` (https://elixir.bootlin.com/linux/v7.1.2/source/include/linux/memcontrol.h#L1519). 

   - Again, this calls ```folio_lruvec_lock_irqsave``` (https://elixir.bootlin.com/linux/v7.1.2/source/mm/memcontrol.c#L1443) which gets the competent ```struct lruvec``` for the folio
(recall that ```struct lruvec``` contains the multiple LRU lists) and holds that ```lruvec```'s ```lru_lock``` once and for all folios in that batch (hopefully, see source).

 * After doing the move-to-LRU operation this flags the folio as belonging to an LRU.

 * After the ```folio_batch``` has been completely scanned, it will release the ```lruvec```'s ```lru_lock``` (if effectively held) and, more importantly, it does **```folios_put```** of the whole
 ```folio_batch``` passed in.

The ```folios_put``` function calls ```folios_put_refs(fbatch, NULL)```, causing the decrement by one of each ```folio``` in the ```folio_batch``` and the **reinitialization**/**emptying** of the 
passed per-CPU ```folio_batch```. 

Before seeing the operations of ```folios_put_batch``` let's get back to the "wrapper" of the core function: **```__folio_batch_add_and_move```** https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L182:
 
 * It receives as parameter the per-CPU ```folio_batch``` being one of the ```cpu_fbatches```, the ```folio``` on which we shall do the LRU operation and the ```move_fn``` callback.

 * It does a **```folio_get(folio)```**, **thus incrementing the refcount** of the **```folio```** (**THIS IS IMPORTANT**).

 * It gets the ```local_lock``` of the per-CPU ```cpu_fbatches```, protecting all the ```folio_batch```es in that per-CPU variable of type ```struct cpu_fbatches``` (multiple interleaved kernel contol paths
 on the same CPU, preemption enabled, bla bla... so a lock protecting data structures is needed). It may disable interrupts, but we don't care about that.

 * It adds (and will succeed 100%) the ```folio``` to the per-CPU ```folio_batch``` passed in to the fn. What happens is that:
   - if the ```folio_batch_add``` function returns 0 this means that after this last insertion, the per-CPU fbatch is full: DRAINING of the batch is necessary right now, 
   by calling the core fn ```folio_batch_move_lru```

   - otherwise, do nothing and keep the ```folio``` in that per-CPU batch, when it gets full, it also gets drained as explained above.

 * Release the previously-taken, ```folio_batch```-protecting, ```local_lock``` of per-CPU ```cpu_fbatches``` data structure, also reenable irqs if previously disabled.

```__folio_batch_add_and_move``` is called using the macro ```folio_batch_add_and_move``` (https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L204) which allows to make code more readable (for
example here: https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L339). Per-CPU batches are named as the ```move_fn```s.

**RECALL THAT WHILE THE ```folio``` is in the per-CPU ```folio_batch```, its refcount is greater than 0!!**

Now, lets get back-on-track with **```folios_put_refs```**, which was also described in previous section. Lets not forget this context: here it gets called by ```folio_batch_move_lru```. The job
of ```folios_put_refs``` here is to decrement the refcount by 1 of each ```folio``` of the drained ```folio_batch``` because it was ```folio_get```-ted before (see ```__folio_batch_add_and_move```), that is,
when inserting that ```folio``` in that ```folio_batch```, and since this ```folio``` is not in the per-CPU ```folio_batch``` anymore, its refcount can be decremented. If the ```folio``` refcnt gets down to 0,
```free_unref_folios``` is called to free all ```folio```s of that batch such that its refcnt is 0.

**Observation**: the ```folio``` being inserted in the LRU doesn't increment its reference count. Why is that? By looking at the code of ```folios_put_refs```, and 
in particular when it calls ```__page_cache_release``` (https://elixir.bootlin.com/linux/v7.1.2/source/mm/swap.c#L73), the reference count is already down to 0.

**The moral of the story**: when doing the ```munmap``` of anonymous private page (as explained above, in prev. sections), it may happen that after the ```folios_put_refs``` (called by 
```free_pages_and_swap_cache```), one or more refcount of the folios of the in-place built ```folio_batch``` (**not** the per-CPU ones!!!! **But** the ```free_pages_and_swap_cache``` created one) 
is **NOT** 0 because it may be temporarily sitting in
a **per-CPU** ```folio_batch``` (```cpu_fbatches```), waiting for that batch to be drained, when it gets full. 
And thus, it is not possible to do ```free_unref_folios``` on the ```current``` task's control path which called
```munmap``` "synchronously".

**Impact on testing**: see 07-testing-and-examples.md

**How I discovered this**: by placing ```BUG()``` in the ```free_unref_folios``` hook, to get the call trace to discover where it got called (saw ```__folio_batch_add_and_move```)

**Sources**:
 - https://richardweiyang-2.gitbook.io/kernel-exploring/nei-cun-guan-li/00-index-1/04-pfra
 - https://www.kernel.org/doc/gorman/html/understand/understand013.html#toc72 "Manipulating LRU lists" for the old ```pagevec``` brief explaination
 - Reading the kernel source itself (mm/swap.c mostly)
