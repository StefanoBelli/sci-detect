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
