# Page mobility

The kernel allows for physical pages to be moved around, if somewhat needed.

See ```enum migratetype```: https://elixir.bootlin.com/linux/v6.14.11/source/include/linux/mmzone.h#L48

In particular, **user pages** are grouped in the ```migratetype::MIGRATE_MOVABLE```,
because the kernel can do memory compaction and just change the physical address
pointed by PTEs.

Even though memory compaction is **rare** (especially with non-NUMA machines 
and THP disabled) it may happen that a "user" physical page is moved around,
leading the ```pgtrack``` mechanism to wrong results.

The kernel daemon **kcompactd** does this, which applies an algorithm to reduce
memory fragmentation by moving movable pages (user pages are one such type).

## Is this really an issue???

The swap / file-backed page writeout / NUMA migration / mobility of pages in general is an issue? They require deeper inspection and implementation for sure.

### Relation with swapping and NUMA migration

This is somewhat related to the swapping of anonymous pages/file backed memory,
because they both imply that the actual page content is moved under our nose,
so the "malicious" content still around but not tracked anymore.

### Possible solutions ???? Not really

 * **Disable memory compaction** entirely (e.g. with sysctl or procfs)

```
# echo 0 > /proc/sys/vm/compaction_proactiveness
```
 It shouldn't be really needed if we have already disabled transparent
 hugepages, since it is the major subsystem that can benefit from physical
 memory compaction.

 * **Add support to GUP** ```pin_user_pages```/```get_user_pages``` 
 (which may solve also the swap and NUMA issues) in code

### Issues with adding GUP support

The page untracking mechanism is based on ```free_unref_folios``` which is
called when refcount of the folio reaches 0. If we call 
```pin_user_pages_fast``` (which **DOES NOT REQUIRE HOLDING THE mmap lock**),
then the page will never be freed because we will never know when the user
did a munmap on it since the refcount will be 2 (ours + user's), but when
user does a munmap it reaches 1... page still there and pinned, won't be
unpinned and freed (we decrement the refcount if page is freed, but we are
"holding" that last reference that would cause the free itself)

However, there is a major issue with these functions: they **might sleep**, and
we're in **kprobe** context.

 * ```pin_user_pages_fast``` (https://elixir.bootlin.com/linux/v6.14.11/source/mm/gup.c#L3528)

 * ```gup_fast_fallback``` (https://elixir.bootlin.com/linux/v6.14.11/source/mm/gup.c#L3391)

 * https://elixir.bootlin.com/linux/v6.14.11/source/mm/gup.c#L3408

 ```FOLL_FAST_ONLY```: if not enabled then we might sleep (acquire the
 mmap read lock), otherwise we risk to not pin user page.

### Sources
 * https://lwn.net/Articles/807108/
 * https://lwn.net/Articles/368869/
 * https://docs.kernel.org/core-api/pin_user_pages.html
 * https://lwn.net/Articles/817905/
 * https://www.reddit.com/r/Ubuntu/comments/rcgt6z/how_to_disable_kcompactd_systemwide_permanently/
