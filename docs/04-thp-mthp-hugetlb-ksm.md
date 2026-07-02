# Disabling Transparent hugepages (THP)

This feature allows for reduction in amount of TLB entries, 
therefore reducing costly page table walks by the MMU.

This is done transparently*

## Turn-off completely

Kernel will never attempt to transparently "merge" PTEs transparently or other stuff...

```bash
#!/bin/bash

echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled
```

**IMPORTANT NOTE**: *khugepaged directly affected, no need to change anything in 
```/sys/kernel/mm/transparent_hugepage/khugepaged/``` subdir*

## Leaving the possibility to the apps of madvising the kernel for THP

Kernel will try to do THP when application requests to do so (via ```madvise``` system call)

```bash
#!/bin/bash

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
echo madvise > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/shmem_enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/enabled
echo never > /sys/kernel/mm/transparent_hugepage/hugepages-*kB/shmem_enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/enabled
echo inherit > /sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled
```

**NOTE**: *this appears to be the default config on Gentoo and Fedora, while Arch seems to
"always" allow THP*

# Disabling explicit hugepages 

This is the situation where a user may explicitly request a 2MB (even 1GB?? not sure...) huge page to themselves.

Normally, the kernel will not allow the above-mentioned situation (due to fast memory exhaustion?),
and on a normal workload the number of (globally) assignable huge pages to user process is 0
(default on most distros, I suppose).

However, the sysadmin may choose to allow them up to a certain number.

### 3 ways to check

* Via ```/proc/meminfo```

```grep -i HugePages_Total /proc/meminfo```

* Via ```/proc/sys/vm/nr_hugepages```

```cat /proc/sys/vm/nr_hugepages```

* Via sysctl options

```sysctl vm.nr_hugepages```

#### Sources
 * https://access.redhat.com/solutions/46111
 * https://publish.obsidian.md/mm/Transparent+Huge+Pages+(THP)

===

# Kernel Samepage Merging (KSM)

It's a feature of the Linux kernel, present in it since 2.6.32, enabled by CONFIG_KSM=y.

KSM is a memory-saving de-duplication feature, KSM was originally developed for use with KVM,
but it can be useful to any application which generates many instances of the same data.

Basically the ksmd daemon periodically scans memory, looking for pages with same perms and same
content. PTEs pointing those different pages are pointed towards a unique "shared" common page,
in CoW.

KSM only merges anonymous (private) pages, never pagecache (file) pages. 
KSM’s merged pages were originally locked into kernel memory, 
but can now be swapped out just like other user pages

KSM **only** operates on those areas of address space which an application has advised to be 
likely candidates for merging, by using the madvise(2) system call:

```int madvise(addr, length, MADV_MERGEABLE)```
```int madvise(addr, length, MADV_UNMERGEABLE)```

The KSM daemon is controlled by sysfs files in /sys/kernel/mm/ksm/, 
readable by all but writable only by root.

To check if KSM is enabled: /sys/kernel/mm/ksm/run

### Adoption

Not widely used by common distros. Anyway, user who wants to profit from memory-saving
feature of KSM must use ```madvise```, even if ksm is enabled.
